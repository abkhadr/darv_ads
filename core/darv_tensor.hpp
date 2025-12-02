#ifndef DARV_TENSOR_HPP
#define DARV_TENSOR_HPP

#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>

namespace darv {
namespace autograd {

// Shape representation
using Shape = std::vector<size_t>;

// Tensor: multi-dimensional array مع autograd
class Tensor : public std::enable_shared_from_this<Tensor> {
private:
    std::vector<double> data_;
    std::vector<double> grad_;
    Shape shape_;
    size_t size_;
    
    bool requires_grad_;
    std::string name_;
    
    // حساب الـ size من الـ shape
    static size_t compute_size(const Shape& shape) {
        if (shape.empty()) return 0;
        return std::accumulate(shape.begin(), shape.end(), 1ULL, std::multiplies<size_t>());
    }
    
    // التحقق من توافق الأشكال
    static bool shapes_compatible(const Shape& s1, const Shape& s2) {
        if (s1.size() != s2.size()) return false;
        for (size_t i = 0; i < s1.size(); i++) {
            if (s1[i] != s2[i]) return false;
        }
        return true;
    }

public:
    // ⚡ CRITICAL FIX: Make these public so nn_layers can access them
    std::vector<std::shared_ptr<Tensor>> inputs_;
    std::function<void()> backward_fn_;

    // Constructors
    Tensor(const Shape& shape, bool requires_grad = true)
        : shape_(shape), size_(compute_size(shape)), 
          requires_grad_(requires_grad), name_("") {
        data_.resize(size_, 0.0);
        if (requires_grad_) {
            grad_.resize(size_, 0.0);
        }
        backward_fn_ = [](){};
    }
    
    Tensor(const std::vector<double>& data, const Shape& shape, bool requires_grad = true)
        : data_(data), shape_(shape), size_(compute_size(shape)),
          requires_grad_(requires_grad), name_("") {
        if (data_.size() != size_) {
            throw std::runtime_error("Data size doesn't match shape");
        }
        if (requires_grad_) {
            grad_.resize(size_, 0.0);
        }
        backward_fn_ = [](){};
    }
    
    // تهيئة عشوائية
    static std::shared_ptr<Tensor> randn(const Shape& shape, bool requires_grad = true) {
        auto tensor = std::make_shared<Tensor>(shape, requires_grad);
        for (auto& val : tensor->data_) {
            // Box-Muller transform للتوزيع الطبيعي
            double u1 = (double)rand() / RAND_MAX;
            double u2 = (double)rand() / RAND_MAX;
            val = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
        }
        return tensor;
    }
    
    // تهيئة بالأصفار
    static std::shared_ptr<Tensor> zeros(const Shape& shape, bool requires_grad = true) {
        return std::make_shared<Tensor>(shape, requires_grad);
    }
    
    // تهيئة بالآحاد
    static std::shared_ptr<Tensor> ones(const Shape& shape, bool requires_grad = true) {
        auto tensor = std::make_shared<Tensor>(shape, requires_grad);
        std::fill(tensor->data_.begin(), tensor->data_.end(), 1.0);
        return tensor;
    }
    
    // Getters
    const std::vector<double>& data() const { return data_; }
    std::vector<double>& data() { return data_; }
    const std::vector<double>& grad() const { return grad_; }
    std::vector<double>& grad() { return grad_; }
    const Shape& shape() const { return shape_; }
    size_t size() const { return size_; }
    bool requires_grad() const { return requires_grad_; }
    std::string name() const { return name_; }
    
    void set_name(const std::string& name) { name_ = name; }
    
    // Element access
    double& operator[](size_t i) { return data_[i]; }
    const double& operator[](size_t i) const { return data_[i]; }
    
    // ==================== العمليات الأساسية (Helper Methods) ====================
    
    // إضافة (element-wise) - كـ method
    std::shared_ptr<Tensor> add(std::shared_ptr<Tensor> other) {
        if (!shapes_compatible(shape_, other->shape_)) {
            throw std::runtime_error("Shape mismatch in addition");
        }
        
        auto result = std::make_shared<Tensor>(shape_, requires_grad_ || other->requires_grad_);
        
        for (size_t i = 0; i < size_; i++) {
            result->data_[i] = data_[i] + other->data_[i];
        }
        
        result->inputs_ = {shared_from_this(), other};
        
        result->backward_fn_ = [this, other, result]() {
            if (this->requires_grad_) {
                for (size_t i = 0; i < this->size_; i++) {
                    this->grad_[i] += result->grad_[i];
                }
            }
            if (other->requires_grad_) {
                for (size_t i = 0; i < other->size_; i++) {
                    other->grad_[i] += result->grad_[i];
                }
            }
        };
        
        return result;
    }
    
    // ضرب (element-wise) - كـ method
    std::shared_ptr<Tensor> multiply(std::shared_ptr<Tensor> other) {
        if (!shapes_compatible(shape_, other->shape_)) {
            throw std::runtime_error("Shape mismatch in multiplication");
        }
        
        auto result = std::make_shared<Tensor>(shape_, requires_grad_ || other->requires_grad_);
        
        for (size_t i = 0; i < size_; i++) {
            result->data_[i] = data_[i] * other->data_[i];
        }
        
        result->inputs_ = {shared_from_this(), other};
        
        result->backward_fn_ = [this, other, result]() {
            if (this->requires_grad_) {
                for (size_t i = 0; i < this->size_; i++) {
                    this->grad_[i] += other->data_[i] * result->grad_[i];
                }
            }
            if (other->requires_grad_) {
                for (size_t i = 0; i < other->size_; i++) {
                    other->grad_[i] += this->data_[i] * result->grad_[i];
                }
            }
        };
        
        return result;
    }
    
    // ضرب في scalar - كـ method
    std::shared_ptr<Tensor> multiply_scalar(double scalar) {
        auto result = std::make_shared<Tensor>(shape_, requires_grad_);
        
        for (size_t i = 0; i < size_; i++) {
            result->data_[i] = data_[i] * scalar;
        }
        
        result->inputs_ = {shared_from_this()};
        
        result->backward_fn_ = [this, scalar, result]() {
            if (this->requires_grad_) {
                for (size_t i = 0; i < this->size_; i++) {
                    this->grad_[i] += scalar * result->grad_[i];
                }
            }
        };
        
        return result;
    }
    
    // إضافة دالة pow
    std::shared_ptr<Tensor> pow(double exponent) {
        auto result = std::make_shared<Tensor>(shape_, requires_grad_);
        
        for (size_t i = 0; i < size_; i++) {
            result->data_[i] = std::pow(data_[i], exponent);
        }
        
        result->inputs_ = {shared_from_this()};
        
        result->backward_fn_ = [this, exponent, result]() {
            if (this->requires_grad_) {
                for (size_t i = 0; i < this->size_; i++) {
                    // d/dx(x^n) = n * x^(n-1)
                    this->grad_[i] += exponent * std::pow(this->data_[i], exponent - 1) * result->grad_[i];
                }
            }
        };
        
        return result;
    }
    
    // ==================== Matrix Operations ====================
    
    // Matrix multiplication (2D only for now)
    std::shared_ptr<Tensor> matmul(std::shared_ptr<Tensor> other) {
        if (shape_.size() != 2 || other->shape_.size() != 2) {
            throw std::runtime_error("matmul requires 2D tensors");
        }
        
        size_t m = shape_[0];
        size_t k = shape_[1];
        size_t n = other->shape_[1];
        
        if (k != other->shape_[0]) {
            throw std::runtime_error("Incompatible dimensions for matmul");
        }
        
        auto result = std::make_shared<Tensor>(Shape{m, n}, requires_grad_ || other->requires_grad_);
        
        // C = A @ B
        for (size_t i = 0; i < m; i++) {
            for (size_t j = 0; j < n; j++) {
                double sum = 0.0;
                for (size_t l = 0; l < k; l++) {
                    sum += data_[i * k + l] * other->data_[l * n + j];
                }
                result->data_[i * n + j] = sum;
            }
        }
        
        result->inputs_ = {shared_from_this(), other};
        
        result->backward_fn_ = [this, other, result, m, k, n]() {
            // dL/dA = dL/dC @ B^T
            if (this->requires_grad_) {
                for (size_t i = 0; i < m; i++) {
                    for (size_t j = 0; j < k; j++) {
                        double grad_sum = 0.0;
                        for (size_t l = 0; l < n; l++) {
                            grad_sum += result->grad_[i * n + l] * other->data_[j * n + l];
                        }
                        this->grad_[i * k + j] += grad_sum;
                    }
                }
            }
            
            // dL/dB = A^T @ dL/dC
            if (other->requires_grad_) {
                for (size_t i = 0; i < k; i++) {
                    for (size_t j = 0; j < n; j++) {
                        double grad_sum = 0.0;
                        for (size_t l = 0; l < m; l++) {
                            grad_sum += this->data_[l * k + i] * result->grad_[l * n + j];
                        }
                        other->grad_[i * n + j] += grad_sum;
                    }
                }
            }
        };
        
        return result;
    }
    
    // ==================== Reduction Operations ====================
    
    // Sum all elements
    std::shared_ptr<Tensor> sum() {
        auto result = std::make_shared<Tensor>(Shape{1}, requires_grad_);
        
        double sum_val = 0.0;
        for (double val : data_) {
            sum_val += val;
        }
        result->data_[0] = sum_val;
        
        result->inputs_ = {shared_from_this()};
        
        result->backward_fn_ = [this, result]() {
            if (this->requires_grad_) {
                for (size_t i = 0; i < this->size_; i++) {
                    this->grad_[i] += result->grad_[0];
                }
            }
        };
        
        return result;
    }
    
    // Mean of all elements
    std::shared_ptr<Tensor> mean() {
        auto sum_tensor = sum();
        return sum_tensor->multiply_scalar(1.0 / size_);
    }
    
    // Flatten to 1D
    std::shared_ptr<Tensor> flatten() {
        return reshape(Shape{size_});
    }
    
    // ==================== Activation Functions ====================
    
    // ReLU
    std::shared_ptr<Tensor> relu() {
        auto result = std::make_shared<Tensor>(shape_, requires_grad_);
        
        for (size_t i = 0; i < size_; i++) {
            result->data_[i] = std::max(0.0, data_[i]);
        }
        
        result->inputs_ = {shared_from_this()};
        
        result->backward_fn_ = [this, result]() {
            if (this->requires_grad_) {
                for (size_t i = 0; i < this->size_; i++) {
                    this->grad_[i] += (this->data_[i] > 0) ? result->grad_[i] : 0.0;
                }
            }
        };
        
        return result;
    }
    
    // Sigmoid
    std::shared_ptr<Tensor> sigmoid() {
        auto result = std::make_shared<Tensor>(shape_, requires_grad_);
        
        for (size_t i = 0; i < size_; i++) {
            result->data_[i] = 1.0 / (1.0 + std::exp(-data_[i]));
        }
        
        result->inputs_ = {shared_from_this()};
        
        result->backward_fn_ = [this, result]() {
            if (this->requires_grad_) {
                for (size_t i = 0; i < this->size_; i++) {
                    double s = result->data_[i];
                    this->grad_[i] += s * (1.0 - s) * result->grad_[i];
                }
            }
        };
        
        return result;
    }
    
    // Tanh
    std::shared_ptr<Tensor> tanh_() {
        auto result = std::make_shared<Tensor>(shape_, requires_grad_);
        
        for (size_t i = 0; i < size_; i++) {
            result->data_[i] = std::tanh(data_[i]);
        }
        
        result->inputs_ = {shared_from_this()};
        
        result->backward_fn_ = [this, result]() {
            if (this->requires_grad_) {
                for (size_t i = 0; i < this->size_; i++) {
                    double t = result->data_[i];
                    this->grad_[i] += (1.0 - t * t) * result->grad_[i];
                }
            }
        };
        
        return result;
    }
    
    // ==================== Backpropagation ====================
    
    void backward() {
        // Build topological order
        std::vector<std::shared_ptr<Tensor>> topo;
        std::set<std::shared_ptr<Tensor>> visited;
        
        std::function<void(std::shared_ptr<Tensor>)> build_topo;
        build_topo = [&](std::shared_ptr<Tensor> t) {
            if (visited.find(t) == visited.end()) {
                visited.insert(t);
                for (auto& input : t->inputs_) {
                    build_topo(input);
                }
                topo.push_back(t);
            }
        };
        
        build_topo(shared_from_this());
        
        // Initialize gradient
        std::fill(grad_.begin(), grad_.end(), 1.0);
        
        // Apply chain rule in reverse order
        for (auto it = topo.rbegin(); it != topo.rend(); ++it) {
            (*it)->backward_fn_();
        }
    }
    
    // Reset gradients
    void zero_grad() {
        std::fill(grad_.begin(), grad_.end(), 0.0);
        for (auto& input : inputs_) {
            input->zero_grad();
        }
    }
    
    // ==================== Utilities ====================
    
    // Print tensor
    void print(const std::string& prefix = "") const {
        std::cout << prefix;
        if (!name_.empty()) {
            std::cout << name_ << " ";
        }
        std::cout << "Tensor(shape=[";
        for (size_t i = 0; i < shape_.size(); i++) {
            std::cout << shape_[i];
            if (i < shape_.size() - 1) std::cout << ", ";
        }
        std::cout << "])" << std::endl;
        
        // Print data (first few elements if large)
        size_t print_limit = std::min(size_, size_t(10));
        std::cout << prefix << "data: [";
        for (size_t i = 0; i < print_limit; i++) {
            std::cout << std::fixed << std::setprecision(4) << data_[i];
            if (i < print_limit - 1) std::cout << ", ";
        }
        if (size_ > print_limit) std::cout << ", ...";
        std::cout << "]" << std::endl;
        
        if (requires_grad_ && !grad_.empty()) {
            std::cout << prefix << "grad: [";
            for (size_t i = 0; i < print_limit; i++) {
                std::cout << std::fixed << std::setprecision(4) << grad_[i];
                if (i < print_limit - 1) std::cout << ", ";
            }
            if (size_ > print_limit) std::cout << ", ...";
            std::cout << "]" << std::endl;
        }
    }
    
    // Reshape with gradient support
    std::shared_ptr<Tensor> reshape(const Shape& new_shape) {
        if (compute_size(new_shape) != size_) {
            throw std::runtime_error("New shape size doesn't match tensor size");
        }
        
        auto result = std::make_shared<Tensor>(data_, new_shape, requires_grad_);
        
        // ⚡ CRITICAL FIX: Setup backward connection
        result->inputs_ = {shared_from_this()};
        result->backward_fn_ = [this, result]() {
            if (this->requires_grad_) {
                // Gradient flows back unchanged (just reshapes)
                for (size_t i = 0; i < this->size_; i++) {
                    this->grad_[i] += result->grad_[i];
                }
            }
        };
        
        return result;
    }
};

// ==================== Global Operators (للعمل مع shared_ptr) ====================

// operator+ للـ tensors
inline std::shared_ptr<Tensor> operator+(std::shared_ptr<Tensor> a, std::shared_ptr<Tensor> b) {
    return a->add(b);
}

// operator* للـ tensors (element-wise)
inline std::shared_ptr<Tensor> operator*(std::shared_ptr<Tensor> a, std::shared_ptr<Tensor> b) {
    return a->multiply(b);
}

// operator* للـ tensor مع scalar (tensor * scalar)
inline std::shared_ptr<Tensor> operator*(std::shared_ptr<Tensor> tensor, double scalar) {
    return tensor->multiply_scalar(scalar);
}

// operator* للـ scalar مع tensor (scalar * tensor)
inline std::shared_ptr<Tensor> operator*(double scalar, std::shared_ptr<Tensor> tensor) {
    return tensor->multiply_scalar(scalar);
}

} // namespace autograd
} // namespace darv

#endif // DARV_TENSOR_HPP