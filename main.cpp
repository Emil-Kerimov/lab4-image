#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <limits>

class Image {
private:
    size_t width_;
    size_t height_;
    std::vector<unsigned char> pixels_;
    unsigned char max_val_;

    // Допоміжна функція для ігнорування коментарів
    void skip_comments(std::ifstream& file) {
        char c;
        file >> std::ws;
        while (file.peek() == '#') {
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            file >> std::ws;
        }
    }

public:
    // Конструктори
    Image() : width_(0), height_(0), max_val_(255) {}

    Image(size_t width, size_t height, unsigned char initial_value = 0)
        : width_(width), height_(height), pixels_(width * height, initial_value), max_val_(255) {}

    // Методи для отримання розмірів
    size_t width() const { return width_; }
    size_t height() const { return height_; }

    // Доступ до пікселів з перевіркою меж
    unsigned char& at(size_t i, size_t j) {
        if (i >= height_ || j >= width_) {
            throw std::out_of_range("Image::at(): indices out of range");
        }
        return pixels_[i * width_ + j];
    }

    const unsigned char& at(size_t i, size_t j) const {
        if (i >= height_ || j >= width_) {
            throw std::out_of_range("Image::at(): indices out of range");
        }
        return pixels_[i * width_ + j];
    }

    // Читання з файлу
    bool readFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Cannot open file: " << filename << std::endl;
            return false;
        }

        std::string magic;
        file >> magic;
        skip_comments(file);

        if (magic != "P2") {
            return false;
        }

        size_t width, height;
        file >> width >> height;
        skip_comments(file);

        unsigned int max_val;
        file >> max_val;
        skip_comments(file);

        if (max_val > 255) {
            return false;
        }

        std::vector<unsigned char> pixels(width * height);
        for (auto& pixel : pixels) {
            int val;
            if (!(file >> val)) {
                return false;
            }
            if (val < 0 || val > max_val) {
                return false;
            }
            pixel = static_cast<unsigned char>(val);
        }

        width_ = width;
        height_ = height;
        max_val_ = static_cast<unsigned char>(max_val);
        pixels_ = std::move(pixels);

        return true;
    }

    bool writeToFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        file << "P2\n";
        file << width_ << " " << height_ << "\n";
        file << static_cast<int>(max_val_) << "\n";

        const size_t values_per_line = 16; // Для кращого читання

        for (size_t i = 0; i < pixels_.size(); ++i) {
            file << static_cast<int>(pixels_[i]) << " ";
            if ((i + 1) % values_per_line == 0 || i == pixels_.size() - 1) {
                file << "\n";
            }
        }

        return true;
    }
};
int main() {
    // Створення зображення та базові операції
    Image img1(10, 10);
    img1.at(0, 0) = 255;
    img1.at(9, 9) = 128;
    if (img1.width() != 10 || img1.height() != 10) {
        std::cerr << "Test 1 failed: wrong dimensions\n";
        return 1;
    }

    // Запис і читання
    if (!img1.writeToFile("test1.pgm")) {
        std::cerr << "Test 2 failed: write error\n";
        return 1;
    }

    Image img2;
    if (!img2.readFromFile("test1.pgm")) {
        std::cerr << "Test 2 failed: read error\n";
        return 1;
    }

    //Перевірка меж
    try {
        img1.at(10, 10) = 0;
        std::cerr << "Test 3 failed: no exception thrown\n";
        return 1;
    } catch (const std::out_of_range& e) {
        std::cerr << "out_of_range error: " << e.what() << std::endl;
    }

    Image img;
    if (!img.readFromFile("balloons.ascii.pgm")) {
        std::cerr << "Failed to read image\n";
        return 1;
    }

    if (!img.writeToFile("balloonsout.ascii.pgm")) {
        std::cerr << "Failed to write image\n";
        return 1;
    }

    std::cout << "All operations completed successfully!\n";
    return 0;
}