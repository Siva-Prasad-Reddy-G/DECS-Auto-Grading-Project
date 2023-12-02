#include "FileQueue.h"

FileQueue::FileQueue(const std::string& filename) : filename_(filename), size_(0) {
    std::ifstream file(filename_);
    if (file.is_open()) {
        std::string line;
        while (getline(file, line)) {
            size_++;
        }
        file.close();
    } else {
        std::cerr << "Unable to open file for reading!" << std::endl;
    }
}

void FileQueue::push(const std::string& data) {
    std::ofstream file(filename_, std::ios::app);
    if (file.is_open()) {
        file << data << std::endl;
        file.close();
        size_++;
    } else {
        std::cerr << "Unable to open file for writing!" << std::endl;
    }
}

std::string FileQueue::pop() {
    std::string data;
    std::ifstream file(filename_);
    if (file.is_open()) {
        if (getline(file, data)) {
            file.close();
            removeFirstLine();
            size_--;
            return data;
        }
        file.close();
    } else {
        std::cerr << "Unable to open file for reading!" << std::endl;
    }
    return "";
}

bool FileQueue::isEmpty() const {
    std::ifstream file(filename_);
    return file.peek() == std::ifstream::traits_type::eof();
}

int FileQueue::size() const {
    return size_;
}

int FileQueue::findPosition(const std::string& element) {
    std::ifstream file(filename_);
    if (file.is_open()) {
        std::string data;
        int position = 1;
        while (getline(file, data)) {
            if (data == element) {
                file.close();
                return position;
            }
            position++;
        }
        file.close();
    } else {
        std::cerr << "Unable to open file for reading!" << std::endl;
    }
    return -1;
}

void FileQueue::removeFirstLine() {
    std::ifstream inputFile(filename_);
    if (!inputFile.is_open()) {
        std::cerr << "Unable to open file for reading!" << std::endl;
        return;
    }

    std::string line;
    getline(inputFile, line); // Read the first line

    std::ofstream outputFile("temp.txt");
    if (!outputFile.is_open()) {
        std::cerr << "Unable to open temporary file for writing!" << std::endl;
        inputFile.close();
        return;
    }

    while (getline(inputFile, line)) {
        outputFile << line << std::endl;
    }

    outputFile.close();
    inputFile.close();

    if (remove(filename_.c_str()) != 0) {
        std::cerr << "Error removing file!" << std::endl;
        return;
    }
    if (rename("temp.txt", filename_.c_str()) != 0) {
        std::cerr << "Error renaming file!" << std::endl;
    }
}

void FileQueue::removeElement(const std::string& element) {
    std::ifstream inputFile(filename_);
    if (!inputFile.is_open()) {
        std::cerr << "Unable to open file for reading!" << std::endl;
        return;
    }

    std::string line;
    std::ofstream outputFile("temp.txt");
    bool found = false;

    while (getline(inputFile, line)) {
        if (line == element && !found) {
            found = true;
            continue;
        }
        outputFile << line << std::endl;
    }

    outputFile.close();
    inputFile.close();

    if (!found) {
        std::cerr << "Element not found in the queue!" << std::endl;
        remove("temp.txt"); // Delete the temp file if element not found
        return;
    }

    if (remove(filename_.c_str()) != 0) {
        std::cerr << "Error removing file!" << std::endl;
        return;
    }
    if (rename("temp.txt", filename_.c_str()) != 0) {
        std::cerr << "Error renaming file!" << std::endl;
    }
}

