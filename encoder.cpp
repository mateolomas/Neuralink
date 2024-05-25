#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <queue>
#include <bitset>
#include <cstdint>

// Structure to hold WAV file header
struct WavHeader {
    char riff[4];             // "RIFF"
    uint32_t overall_size;    // overall size of file in bytes
    char wave[4];             // "WAVE"
    char fmt_chunk_marker[4]; // "fmt " string with trailing null char
    uint32_t length_of_fmt;   // length of the format data
    uint16_t format_type;     // format type
    uint16_t channels;        // number of channels
    uint32_t sample_rate;     // sampling rate (blocks per second)
    uint32_t byterate;        // SampleRate * NumChannels * BitsPerSample/8
    uint16_t block_align;     // NumChannels * BitsPerSample/8
    uint16_t bits_per_sample; // bits per sample, 8- 8bits, 16- 16 bits etc
    char data_chunk_header[4];// "data"
    uint32_t data_size;       // data size
};

std::vector<int16_t> readWavFile(const std::string &filename, WavHeader &header) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        exit(1);
    }

    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    // Check if it's a valid WAV file
    if (std::string(header.riff, 4) != "RIFF" || std::string(header.wave, 4) != "WAVE") {
        std::cerr << "Invalid WAV file: " << filename << std::endl;
        exit(1);
    }

    std::vector<int16_t> audioData(header.data_size / sizeof(int16_t));
    file.read(reinterpret_cast<char*>(audioData.data()), header.data_size);
    return audioData;
}

std::map<int16_t, int> calculateFrequencies(const std::vector<int16_t> &audioData) {
    std::map<int16_t, int> frequencies;
    for (int16_t sample : audioData) {
        frequencies[sample]++;
    }
    return frequencies;
}

struct HuffmanNode {
    int16_t sample;
    int frequency;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(int16_t sample, int frequency) : sample(sample), frequency(frequency), left(nullptr), right(nullptr) {}
};

struct Compare {
    bool operator()(HuffmanNode* l, HuffmanNode* r) {
        return l->frequency > r->frequency;
    }
};

HuffmanNode* buildHuffmanTree(const std::map<int16_t, int> &frequencies) {
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, Compare> minHeap;
    
    for (const auto &pair : frequencies) {
        minHeap.push(new HuffmanNode(pair.first, pair.second));
    }

    while (minHeap.size() > 1) {
        HuffmanNode* left = minHeap.top();
        minHeap.pop();
        HuffmanNode* right = minHeap.top();
        minHeap.pop();

        HuffmanNode* combined = new HuffmanNode(-1, left->frequency + right->frequency);
        combined->left = left;
        combined->right = right;

        minHeap.push(combined);
    }

    return minHeap.top();
}

void generateCodes(HuffmanNode* root, const std::string &str, std::map<int16_t, std::string> &huffmanCodes) {
    if (!root) return;

    if (root->sample != -1) {
        huffmanCodes[root->sample] = str;
    }

    generateCodes(root->left, str + "0", huffmanCodes);
    generateCodes(root->right, str + "1", huffmanCodes);
}

std::string encodeAudioData(const std::vector<int16_t> &audioData, const std::map<int16_t, std::string> &huffmanCodes) {
    std::string encodedData;
    for (int16_t sample : audioData) {
        encodedData += huffmanCodes.at(sample);
    }
    return encodedData;
}

void saveEncodedFile(const std::string &filename, const WavHeader &header, const std::string &encodedData, const std::map<int16_t, std::string> &huffmanCodes) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error creating file: " << filename << std::endl;
        exit(1);
    }

    // Write the WAV header to the file
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write the Huffman codes to the file
    uint32_t huffmanCodeCount = huffmanCodes.size();
    file.write(reinterpret_cast<char*>(&huffmanCodeCount), sizeof(huffmanCodeCount));
    for (const auto &pair : huffmanCodes) {
        file.write(reinterpret_cast<const char*>(&pair.first), sizeof(pair.first));
        uint32_t codeLength = pair.second.size();
        file.write(reinterpret_cast<const char*>(&codeLength), sizeof(codeLength));
        file.write(pair.second.data(), codeLength);
    }

    // Write the encoded data size
    uint32_t encodedDataSize = encodedData.size();
    file.write(reinterpret_cast<char*>(&encodedDataSize), sizeof(encodedDataSize));

    // Write the encoded data as bits
    std::bitset<8> byte;
    for (size_t i = 0; i < encodedData.size(); ++i) {
        byte[i % 8] = encodedData[i] == '1';
        if (i % 8 == 7 || i == encodedData.size() - 1) {
            unsigned char c = static_cast<unsigned char>(byte.to_ulong());
            file.write(reinterpret_cast<const char*>(&c), sizeof(c));
            byte.reset();
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_wav_file> <output_encoded_file>" << std::endl;
        return 1;
    }

    std::string inputFilePath = argv[1];
    std::string outputFilePath = argv[2];

    WavHeader header;
    std::vector<int16_t> audioData = readWavFile(inputFilePath, header);

    std::map<int16_t, int> frequencies = calculateFrequencies(audioData);
    HuffmanNode* huffmanTree = buildHuffmanTree(frequencies);

    std::map<int16_t, std::string> huffmanCodes;
    generateCodes(huffmanTree, "", huffmanCodes);

    std::string encodedData = encodeAudioData(audioData, huffmanCodes);
    saveEncodedFile(outputFilePath, header, encodedData, huffmanCodes);

    std::cout << "Encoding completed." << std::endl;

    return 0;
}