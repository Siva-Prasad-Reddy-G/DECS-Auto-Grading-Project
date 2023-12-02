#ifndef FILEQUEUE_H
#define FILEQUEUE_H

#include <iostream>
#include <fstream>
#include <string>

class FileQueue {
public:
    FileQueue(const std::string& filename);
    void push(const std::string& data);
    std::string pop();
    bool isEmpty() const;
    int size() const;
    int findPosition(const std::string& element);
    void removeElement(const std::string& element);

private:
    std::string filename_;
    int size_;

    void removeFirstLine();
};

#endif /* FILEQUEUE_H */

