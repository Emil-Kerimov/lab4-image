#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <memory>
#include <limits>

class Image {
public:
    struct Pixel {
        unsigned char r, g, b;
        Pixel(unsigned char r = 0, unsigned char g = 0, unsigned char b = 0)
            : r(r), g(g), b(b) {}
    };

    // Інтерфейс стратегії
    class IImageStrategy {
    public:
        virtual ~IImageStrategy() = default;
        virtual bool read(Image& image, const std::string& filename) const = 0;
        virtual bool write(const Image& image, const std::string& filename) const = 0;
    };

    static std::unique_ptr<IImageStrategy> createPgmStrategy() {
        return std::make_unique<PgmStrategy>();
    }

    static std::unique_ptr<IImageStrategy> createPpmStrategy() {
        return std::make_unique<PpmStrategy>();
    }

private:
    size_t width_;
    size_t height_;
    std::vector<Pixel> pixels_;
    unsigned char max_val_;
    // Стратегія для PGM
    class PgmStrategy : public IImageStrategy {
        void skip_comments(std::ifstream& file) const {
            file >> std::ws;
            while (file.peek() == '#') {
                file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                file >> std::ws;
            }
        }

    public:
        bool read(Image& image, const std::string& filename) const override {
            std::ifstream file(filename);
            if (!file.is_open()) return false;

            std::string magic;
            file >> magic;
            skip_comments(file);

            if (magic != "P2") return false;

            size_t width, height;
            file >> width >> height;
            skip_comments(file);

            unsigned int max_val;
            file >> max_val;
            skip_comments(file);

            if (max_val > 255) return false;

            std::vector<Pixel> pixels(width * height);
            for (auto& pixel : pixels) {
                int val;
                if (!(file >> val)) return false;
                if (val < 0 || val > max_val) return false;
                pixel = Pixel(val, val, val);
            }

            image.width_ = width;
            image.height_ = height;
            image.max_val_ = static_cast<unsigned char>(max_val);
            image.pixels_ = std::move(pixels);

            return true;
        }

        bool write(const Image& image, const std::string& filename) const override {
            std::ofstream file(filename);
            if (!file.is_open()) return false;

            file << "P2\n";
            file << image.width_ << " " << image.height_ << "\n";
            file << static_cast<int>(image.max_val_) << "\n";

            const size_t values_per_line = 16;
            for (size_t i = 0; i < image.pixels_.size(); ++i) {
                // Для PGM використовуємо тільки червоний канал
                file << static_cast<int>(image.pixels_[i].r) << " ";
                if ((i + 1) % values_per_line == 0 || i == image.pixels_.size() - 1) {
                    file << "\n";
                }
            }

            return true;
        }
    };

    // Стратегія для PPM
    class PpmStrategy : public IImageStrategy {
        void skip_comments(std::ifstream& file) const {
            file >> std::ws;
            while (file.peek() == '#') {
                file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                file >> std::ws;
            }
        }

    public:
        bool read(Image& image, const std::string& filename) const override {
            std::ifstream file(filename);
            if (!file.is_open()) return false;

            std::string magic;
            file >> magic;
            skip_comments(file);

            if (magic != "P3") return false;

            size_t width, height;
            file >> width >> height;
            skip_comments(file);

            unsigned int max_val;
            file >> max_val;
            skip_comments(file);

            if (max_val > 255) return false;

            std::vector<Pixel> pixels(width * height);
            for (auto& pixel : pixels) {
                int r, g, b;
                if (!(file >> r >> g >> b)) return false;
                if (r < 0 || r > max_val || g < 0 || g > max_val || b < 0 || b > max_val) return false;
                pixel = Pixel(r, g, b);
            }

            image.width_ = width;
            image.height_ = height;
            image.max_val_ = static_cast<unsigned char>(max_val);
            image.pixels_ = std::move(pixels);

            return true;
        }

        bool write(const Image& image, const std::string& filename) const override {
            std::ofstream file(filename);
            if (!file.is_open()) return false;

            file << "P3\n";
            file << image.width_ << " " << image.height_ << "\n";
            file << static_cast<int>(image.max_val_) << "\n";

            for (const auto& pixel : image.pixels_) {
                file << static_cast<int>(pixel.r) << " "
                     << static_cast<int>(pixel.g) << " "
                     << static_cast<int>(pixel.b) << "\n";
            }

            return true;
        }
    };

    std::unique_ptr<IImageStrategy> strategy_;

public:
    Image() : width_(0), height_(0), max_val_(255), strategy_(std::make_unique<PgmStrategy>()) {}

    Image(size_t width, size_t height, unsigned char initial_value = 0)
        : width_(width), height_(height),
          pixels_(width * height, Pixel(initial_value, initial_value, initial_value)),
          max_val_(255), strategy_(std::make_unique<PgmStrategy>()) {}

    size_t width() const { return width_; }
    size_t height() const { return height_; }

    // Доступ до пікселів з перевіркою меж
    Pixel& at(size_t i, size_t j) {
        if (i >= height_ || j >= width_) {
            throw std::out_of_range("Image::at(): indices out of range");
        }
        return pixels_[i * width_ + j];
    }

    const Pixel& at(size_t i, size_t j) const {
        if (i >= height_ || j >= width_) {
            throw std::out_of_range("Image::at(): indices out of range");
        }
        return pixels_[i * width_ + j];
    }

    // Встановлення стратегії
    void setStrategy(std::unique_ptr<IImageStrategy> strategy) {
        strategy_ = std::move(strategy);
    }

    // Читання з файлу
    bool readFromFile(const std::string& filename) {
        // Визначаємо формат за розширенням файлу
        size_t dot_pos = filename.find_last_of('.');
        if (dot_pos != std::string::npos) {
            std::string ext = filename.substr(dot_pos + 1);
            if (ext == "ppm" || ext == "PPM") {
                strategy_ = std::make_unique<PpmStrategy>();
            } else {
                strategy_ = std::make_unique<PgmStrategy>();
            }
        }

        return strategy_->read(*this, filename);
    }

    // Запис у файл
    bool writeToFile(const std::string& filename) const {
        return strategy_->write(*this, filename);
    }
};

int main() {
    // Тестування PGM
    Image img1(2, 2);
    img1.at(0, 0) = Image::Pixel(255, 255, 255);
    img1.at(0, 1) = Image::Pixel(0, 0, 0);
    img1.at(1, 0) = Image::Pixel(128, 128, 128);
    img1.at(1, 1) = Image::Pixel(64, 64, 64);

    if (!img1.writeToFile("test.pgm")) {
        std::cerr << "Failed to write PGM file\n";
        return 1;
    }

    Image img2;
    if (!img2.readFromFile("test.pgm")) {
        std::cerr << "Failed to read PGM file\n";
        return 1;
    }

    // Тестування PPM
    Image img3(2, 2);
    img3.setStrategy(Image::createPpmStrategy());
    img3.at(0, 0) = Image::Pixel(255, 0, 0);    // Червоний
    img3.at(0, 1) = Image::Pixel(0, 255, 0);    // Зелений
    img3.at(1, 0) = Image::Pixel(0, 0, 255);    // Синій
    img3.at(1, 1) = Image::Pixel(255, 255, 0);  // Жовтий

    if (!img3.writeToFile("test.ppm")) {
        std::cerr << "Failed to write PPM file\n";
        return 1;
    }

    Image img4;
    if (!img4.readFromFile("test.ppm")) {
        std::cerr << "Failed to read PPM file\n";
        return 1;
    }

    std::cout << "All tests passed successfully!\n";
    return 0;
}