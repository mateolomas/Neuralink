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

struct HuffmanNode {
    int16_t sample;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(int16_t sample) : sample(sample), left(nullptr), right(nullptr) {}
    HuffmanNode() : sample(-1), left(nullptr), right(nullptr) {}
};

std::map<int16_t, std::string> readHuffmanCodes(std::ifstream &file) {
    std::map<int16_t, std::string> huffmanCodes;
    uint32_t huffmanCodeCount;
    file.read(reinterpret_cast<char*>(&huffmanCodeCount), sizeof(huffmanCodeCount));

    for (uint32_t i = 0; i < huffmanCodeCount; ++i) {
        int16_t sample;
        uint32_t codeLength;
        file.read(reinterpret_cast<char*>(&sample), sizeof(sample));
        file.read(reinterpret_cast<char*>(&codeLength), sizeof(codeLength));

        std::string code(codeLength, '0');
        file.read(&code[0], codeLength);

        huffmanCodes[sample] = code;
    }

    return huffmanCodes;
}

HuffmanNode* buildHuffmanTree(const std::map<int16_t, std::string> &huffmanCodes) {
    HuffmanNode* root = new HuffmanNode();

    for (const auto &pair : huffmanCodes) {
        HuffmanNode* currentNode = root;
        for (char bit : pair.second) {
            if (bit == '0') {
                if (!currentNode->left) {
                    currentNode->left = new HuffmanNode();
                }
                currentNode = currentNode->left;
            } else {
                if (!currentNode->right) {
                    currentNode->right = new HuffmanNode();
                }
                currentNode = currentNode->right;
            }
        }
        currentNode->sample = pair.first;
    }

    return root;
}

std::vector<int16_t> decodeAudioData(const std::string &encodedData, HuffmanNode* root) {
    std::vector<int16_t> audioData;
    HuffmanNode* currentNode = root;

    for (char bit : encodedData) {
        if (bit == '0') {
            currentNode = currentNode->left;
        } else {
            currentNode = currentNode->right;
        }

        if (currentNode->left == nullptr && currentNode->right == nullptr) {
            audioData.push_back(currentNode->sample);
            currentNode = root;
        }
    }

    return audioData;
}

std::string readEncodedData(std::ifstream &file, uint32_t encodedDataSize) {
    std::string encodedData;
    encodedData.reserve(encodedDataSize);

    for (uint32_t i = 0; i < encodedDataSize; ++i) {
        std::bitset<8> byte;
        unsigned char c;
        file.read(reinterpret_cast<char*>(&c), sizeof(c));
        byte = std::bitset<8>(c);

        for (size_t j = 0; j < 8 && encodedData.size() < encodedDataSize; ++j) {
            encodedData.push_back(byte[j] ? '1' : '0');
        }
    }

    return encodedData;
}

void saveWavFile(const std::string &filename, const WavHeader &header, const std::vector<int16_t> &audioData) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error creating file: " << filename << std::endl;
        exit(1);
    }

    // Write the WAV header to the file
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write the audio data to the file
    file.write(reinterpret_cast<const char*>(audioData.data()), audioData.size() * sizeof(int16_t));
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_encoded_file> <output_wav_file>" << std::endl;
        return 1;
    }

    std::string inputFilePath = argv[1];
    std::string outputFilePath = argv[2];

    std::ifstream file(inputFilePath, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << inputFilePath << std::endl;
        return 1;
    }

    // Read the WAV header
    WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    // Read the Huffman codes
    std::map<int16_t, std::string> huffmanCodes = readHuffmanCodes(file);

    // Read the encoded data size
    uint32_t encodedDataSize;
    file.read(reinterpret_cast<char*>(&encodedDataSize), sizeof(encodedDataSize));

    // Read the encoded data
    std::string encodedData = readEncodedData(file, encodedDataSize);

    // Build the Huffman tree
    HuffmanNode* huffmanTree = buildHuffmanTree(huffmanCodes);

    // Decode the audio data
    std::vector<int16_t> audioData = decodeAudioData(encodedData, huffmanTree);

    // Save the decoded audio data to a WAV file
    saveWavFile(outputFilePath, header, audioData);

    std::cout << "Decoding completed." << std::endl;

    return 0;
}
