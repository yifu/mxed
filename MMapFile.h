#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstddef>
#include <cstdint>

class MMapFile {
public:
    // Constructeur : ouvre et mappe le fichier
    MMapFile(const char* path)
        : fd_(-1), data_(nullptr), size_(0), ok_(false)
    {
        fd_ = ::open(path, O_RDWR);
        if (fd_ == -1)
            return;

        struct stat st;
        if (fstat(fd_, &st) != 0)
            return;

        size_ = st.st_size;
        if (size_ == 0)
            return;

        data_ = ::mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (data_ == MAP_FAILED) {
            data_ = nullptr;
            return;
        }

        ok_ = true;
    }

    // Pas de copie (juste pour la sécurité)
    MMapFile(const MMapFile&) = delete;
    MMapFile& operator=(const MMapFile&) = delete;

    // Déplacement possible
    MMapFile(MMapFile&& other)
        : fd_(other.fd_), data_(other.data_), size_(other.size_), ok_(other.ok_)
    {
        other.fd_ = -1;
        other.data_ = nullptr;
        other.size_ = 0;
        other.ok_ = false;
    }

    MMapFile& operator=(MMapFile&& other) {
        if (this != &other) {
            cleanup();
            fd_ = other.fd_;
            data_ = other.data_;
            size_ = other.size_;
            ok_ = other.ok_;

            other.fd_ = -1;
            other.data_ = nullptr;
            other.size_ = 0;
            other.ok_ = false;
        }
        return *this;
    }

    // Destructeur : clean !
    ~MMapFile() {
        cleanup();
    }

    // Pour vérifier si tout s’est bien passé
    bool is_open() const { return ok_; }

    // Accès au pointeur base
    void* data() { return data_; }
    const void* data() const { return data_; }

    // Taille du mapping
    size_t size() const { return size_; }

private:
    int fd_;
    void* data_;
    size_t size_;
    bool ok_;

    void cleanup() {
        if (data_)
            ::munmap(data_, size_);
        if (fd_ != -1)
            ::close(fd_);
        data_ = nullptr;
        fd_ = -1;
        size_ = 0;
        ok_ = false;
    }
};
