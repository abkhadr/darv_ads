#ifndef DARV_OPTIMIZER_HPP
#define DARV_OPTIMIZER_HPP

#include "darv_tensor.hpp"
#include <vector>
#include <memory>
#include <cmath>
#include <unordered_map>

namespace darv {
namespace autograd {

// Base Optimizer class
class Optimizer {
protected:
    std::vector<std::shared_ptr<Tensor>> parameters_;
    double learning_rate_;

public:
    Optimizer(const std::vector<std::shared_ptr<Tensor>>& parameters, double lr = 0.01)
        : parameters_(parameters), learning_rate_(lr) {}
    
    virtual ~Optimizer() = default;
    
    virtual void step() = 0;
    virtual void zero_grad() {
        for (auto& param : parameters_) {
            param->zero_grad();
        }
    }
    
    void set_learning_rate(double lr) { learning_rate_ = lr; }
    double get_learning_rate() const { return learning_rate_; }
};

// ==================== SGD Optimizer ====================
class SGD : public Optimizer {
private:
    double momentum_;
    bool nesterov_;
    std::unordered_map<Tensor*, std::vector<double>> velocity_;

public:
    SGD(const std::vector<std::shared_ptr<Tensor>>& parameters,
        double lr = 0.01,
        double momentum = 0.0,
        bool nesterov = false)
        : Optimizer(parameters, lr), momentum_(momentum), nesterov_(nesterov) {
        
        // Initialize velocity
        if (momentum_ > 0.0) {
            for (auto& param : parameters_) {
                velocity_[param.get()].resize(param->size(), 0.0);
            }
        }
    }
    
    void step() override {
        for (auto& param : parameters_) {
            if (!param->requires_grad()) continue;
            
            auto& grad = param->grad();
            auto& data = param->data();
            
            if (momentum_ > 0.0) {
                auto& v = velocity_[param.get()];
                
                for (size_t i = 0; i < param->size(); i++) {
                    // v = momentum * v + lr * grad
                    v[i] = momentum_ * v[i] + learning_rate_ * grad[i];
                    
                    if (nesterov_) {
                        // Nesterov momentum
                        data[i] -= momentum_ * v[i] + learning_rate_ * grad[i];
                    } else {
                        // Standard momentum
                        data[i] -= v[i];
                    }
                }
            } else {
                // Simple SGD
                for (size_t i = 0; i < param->size(); i++) {
                    data[i] -= learning_rate_ * grad[i];
                }
            }
        }
    }
};

// ==================== Adam Optimizer ====================
class Adam : public Optimizer {
private:
    double beta1_;
    double beta2_;
    double epsilon_;
    int t_; // timestep
    
    std::unordered_map<Tensor*, std::vector<double>> m_; // first moment
    std::unordered_map<Tensor*, std::vector<double>> v_; // second moment

public:
    Adam(const std::vector<std::shared_ptr<Tensor>>& parameters,
         double lr = 0.001,
         double beta1 = 0.9,
         double beta2 = 0.999,
         double epsilon = 1e-8)
        : Optimizer(parameters, lr), 
          beta1_(beta1), beta2_(beta2), epsilon_(epsilon), t_(0) {
        
        // Initialize moments
        for (auto& param : parameters_) {
            m_[param.get()].resize(param->size(), 0.0);
            v_[param.get()].resize(param->size(), 0.0);
        }
    }
    
    void step() override {
        t_++;
        
        for (auto& param : parameters_) {
            if (!param->requires_grad()) continue;
            
            auto& grad = param->grad();
            auto& data = param->data();
            auto& m = m_[param.get()];
            auto& v = v_[param.get()];
            
            for (size_t i = 0; i < param->size(); i++) {
                // Update biased first moment estimate
                m[i] = beta1_ * m[i] + (1.0 - beta1_) * grad[i];
                
                // Update biased second raw moment estimate
                v[i] = beta2_ * v[i] + (1.0 - beta2_) * grad[i] * grad[i];
                
                // Compute bias-corrected first moment estimate
                double m_hat = m[i] / (1.0 - std::pow(beta1_, t_));
                
                // Compute bias-corrected second raw moment estimate
                double v_hat = v[i] / (1.0 - std::pow(beta2_, t_));
                
                // Update parameters
                data[i] -= learning_rate_ * m_hat / (std::sqrt(v_hat) + epsilon_);
            }
        }
    }
};

// ==================== RMSprop Optimizer ====================
class RMSprop : public Optimizer {
private:
    double alpha_;
    double epsilon_;
    std::unordered_map<Tensor*, std::vector<double>> square_avg_;

public:
    RMSprop(const std::vector<std::shared_ptr<Tensor>>& parameters,
            double lr = 0.01,
            double alpha = 0.99,
            double epsilon = 1e-8)
        : Optimizer(parameters, lr), alpha_(alpha), epsilon_(epsilon) {
        
        // Initialize square averages
        for (auto& param : parameters_) {
            square_avg_[param.get()].resize(param->size(), 0.0);
        }
    }
    
    void step() override {
        for (auto& param : parameters_) {
            if (!param->requires_grad()) continue;
            
            auto& grad = param->grad();
            auto& data = param->data();
            auto& sq_avg = square_avg_[param.get()];
            
            for (size_t i = 0; i < param->size(); i++) {
                // Update square average
                sq_avg[i] = alpha_ * sq_avg[i] + (1.0 - alpha_) * grad[i] * grad[i];
                
                // Update parameters
                data[i] -= learning_rate_ * grad[i] / (std::sqrt(sq_avg[i]) + epsilon_);
            }
        }
    }
};

// ==================== AdaGrad Optimizer ====================
class AdaGrad : public Optimizer {
private:
    double epsilon_;
    std::unordered_map<Tensor*, std::vector<double>> sum_squares_;

public:
    AdaGrad(const std::vector<std::shared_ptr<Tensor>>& parameters,
            double lr = 0.01,
            double epsilon = 1e-8)
        : Optimizer(parameters, lr), epsilon_(epsilon) {
        
        // Initialize sum of squares
        for (auto& param : parameters_) {
            sum_squares_[param.get()].resize(param->size(), 0.0);
        }
    }
    
    void step() override {
        for (auto& param : parameters_) {
            if (!param->requires_grad()) continue;
            
            auto& grad = param->grad();
            auto& data = param->data();
            auto& sum_sq = sum_squares_[param.get()];
            
            for (size_t i = 0; i < param->size(); i++) {
                // Accumulate squared gradients
                sum_sq[i] += grad[i] * grad[i];
                
                // Update parameters
                data[i] -= learning_rate_ * grad[i] / (std::sqrt(sum_sq[i]) + epsilon_);
            }
        }
    }
};

// ==================== Learning Rate Schedulers ====================

class LRScheduler {
protected:
    Optimizer* optimizer_;
    double initial_lr_;

public:
    LRScheduler(Optimizer* optimizer)
        : optimizer_(optimizer), initial_lr_(optimizer->get_learning_rate()) {}
    
    virtual ~LRScheduler() = default;
    virtual void step(int epoch) = 0;
};

// Step Decay Scheduler
class StepLR : public LRScheduler {
private:
    int step_size_;
    double gamma_;

public:
    StepLR(Optimizer* optimizer, int step_size, double gamma = 0.1)
        : LRScheduler(optimizer), step_size_(step_size), gamma_(gamma) {}
    
    void step(int epoch) override {
        if (epoch % step_size_ == 0 && epoch > 0) {
            double new_lr = optimizer_->get_learning_rate() * gamma_;
            optimizer_->set_learning_rate(new_lr);
            std::cout << "Learning rate updated to: " << new_lr << std::endl;
        }
    }
};

// Exponential Decay Scheduler
class ExponentialLR : public LRScheduler {
private:
    double gamma_;

public:
    ExponentialLR(Optimizer* optimizer, double gamma = 0.95)
        : LRScheduler(optimizer), gamma_(gamma) {}
    
    void step(int epoch) override {
        double new_lr = initial_lr_ * std::pow(gamma_, epoch);
        optimizer_->set_learning_rate(new_lr);
    }
};

// Cosine Annealing Scheduler
class CosineAnnealingLR : public LRScheduler {
private:
    int T_max_;
    double eta_min_;

public:
    CosineAnnealingLR(Optimizer* optimizer, int T_max, double eta_min = 0.0)
        : LRScheduler(optimizer), T_max_(T_max), eta_min_(eta_min) {}
    
    void step(int epoch) override {
        double new_lr = eta_min_ + (initial_lr_ - eta_min_) * 
                        (1.0 + std::cos(M_PI * epoch / T_max_)) / 2.0;
        optimizer_->set_learning_rate(new_lr);
    }
};

} // namespace autograd
} // namespace darv

#endif // DARV_OPTIMIZER_HPP