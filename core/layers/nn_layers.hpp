#ifndef DARV_NN_LAYERS_HPP
#define DARV_NN_LAYERS_HPP

#include "../darv_tensor.hpp"
#include <random>
#include <cmath>

namespace darv {
namespace nn {

using namespace autograd;

// ==================== Base Layer ====================

class Layer {
protected:
    std::vector<std::shared_ptr<Tensor>> parameters_;
    std::string name_;

public:
    Layer(const std::string& name) : name_(name) {}
    virtual ~Layer() = default;
    
    virtual std::shared_ptr<Tensor> forward(std::shared_ptr<Tensor> input) = 0;
    
    std::vector<std::shared_ptr<Tensor>>& parameters() { return parameters_; }
    const std::string& name() const { return name_; }
};

// ==================== Linear Layer ====================

class Linear : public Layer {
private:
    size_t in_features_;
    size_t out_features_;
    std::shared_ptr<Tensor> weight_;
    std::shared_ptr<Tensor> bias_;

public:
    Linear(size_t in_features, size_t out_features, const std::string& name = "linear")
        : Layer(name), in_features_(in_features), out_features_(out_features) {
        
        // Xavier initialization
        double std = std::sqrt(2.0 / (in_features + out_features));
        
        // Weight: [out_features, in_features]
        weight_ = Tensor::randn(Shape{out_features, in_features}, true);
        for (auto& w : weight_->data()) {
            w *= std;
        }
        weight_->set_name(name + ".weight");
        
        // Bias: [out_features]
        bias_ = Tensor::zeros(Shape{out_features}, true);
        bias_->set_name(name + ".bias");
        
        parameters_.push_back(weight_);
        parameters_.push_back(bias_);
    }
    
    std::shared_ptr<Tensor> forward(std::shared_ptr<Tensor> input) override {
        // Input: [batch_size, in_features] or [in_features]
        // Output: [batch_size, out_features] or [out_features]
        
        auto shape = input->shape();
        bool was_1d = (shape.size() == 1);
        
        if (was_1d) {
            // Single sample: reshape to [1, in_features]
            input = input->reshape(Shape{1, in_features_});
        }
        
        // output = input @ weight.T + bias
        auto weight_t = transpose(weight_);
        auto output = input->matmul(weight_t);
        
        // ⚡ FIX: Bias addition with proper gradient tracking
        size_t batch_size = output->shape()[0];
        auto result = std::make_shared<Tensor>(output->shape(), output->requires_grad());
        
        // Add bias with tracking
        for (size_t i = 0; i < batch_size; i++) {
            for (size_t j = 0; j < out_features_; j++) {
                result->data()[i * out_features_ + j] = 
                    output->data()[i * out_features_ + j] + bias_->data()[j];
            }
        }
        
        // Setup backward for bias
        result->inputs_ = {output, bias_};
        
        // ⚡ FIX: Capture bias_ properly as local variable
        auto bias = bias_;
        auto out_feat = out_features_;
        
        result->backward_fn_ = [output, bias, result, batch_size, out_feat]() {
            if (output->requires_grad()) {
                for (size_t i = 0; i < result->size(); i++) {
                    output->grad()[i] += result->grad()[i];
                }
            }
            if (bias->requires_grad()) {
                // Sum gradients across batch dimension
                for (size_t i = 0; i < batch_size; i++) {
                    for (size_t j = 0; j < out_feat; j++) {
                        bias->grad()[j] += result->grad()[i * out_feat + j];
                    }
                }
            }
        };
        
        return result;
    }
    
private:
    std::shared_ptr<Tensor> transpose(std::shared_ptr<Tensor> t) {
        // Simple 2D transpose
        auto shape = t->shape();
        if (shape.size() != 2) {
            throw std::runtime_error("Transpose requires 2D tensor");
        }
        
        size_t rows = shape[0];
        size_t cols = shape[1];
        
        std::vector<double> data(rows * cols);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                data[j * rows + i] = t->data()[i * cols + j];
            }
        }
        
        return std::make_shared<Tensor>(data, Shape{cols, rows}, t->requires_grad());
    }
};

// ==================== ReLU Activation ====================

class ReLU : public Layer {
public:
    ReLU(const std::string& name = "relu") : Layer(name) {}
    
    std::shared_ptr<Tensor> forward(std::shared_ptr<Tensor> input) override {
        return input->relu();
    }
};

// ==================== Sigmoid Activation ====================

class Sigmoid : public Layer {
public:
    Sigmoid(const std::string& name = "sigmoid") : Layer(name) {}
    
    std::shared_ptr<Tensor> forward(std::shared_ptr<Tensor> input) override {
        return input->sigmoid();
    }
};

// ==================== Tanh Activation ====================

class Tanh : public Layer {
public:
    Tanh(const std::string& name = "tanh") : Layer(name) {}
    
    std::shared_ptr<Tensor> forward(std::shared_ptr<Tensor> input) override {
        return input->tanh_();
    }
};

// ==================== Sequential Container ====================

class Sequential {
private:
    std::vector<std::shared_ptr<Layer>> layers_;
    std::string name_;

public:
    Sequential(const std::string& name = "sequential") : name_(name) {}
    
    void add(std::shared_ptr<Layer> layer) {
        layers_.push_back(layer);
    }
    
    std::shared_ptr<Tensor> forward(std::shared_ptr<Tensor> input) {
        auto output = input;
        for (auto& layer : layers_) {
            output = layer->forward(output);
        }
        return output;
    }
    
    std::vector<std::shared_ptr<Tensor>> parameters() {
        std::vector<std::shared_ptr<Tensor>> all_params;
        for (auto& layer : layers_) {
            auto params = layer->parameters();
            all_params.insert(all_params.end(), params.begin(), params.end());
        }
        return all_params;
    }
    
    void zero_grad() {
        for (auto& param : parameters()) {
            param->zero_grad();
        }
    }
};

// ==================== Loss Functions ====================

class MSELoss {
public:
    static std::shared_ptr<Tensor> compute(std::shared_ptr<Tensor> pred, 
                                           std::shared_ptr<Tensor> target) {
        // ⚡ FIX: Flatten both tensors to ensure shape compatibility
        auto pred_flat = pred->flatten();
        auto target_flat = target->flatten();
        
        // Verify they have same size
        if (pred_flat->size() != target_flat->size()) {
            throw std::runtime_error("MSELoss: prediction and target must have same total size");
        }
        
        // MSE = mean((pred - target)^2)
        auto diff = pred_flat + (-1.0 * target_flat);
        auto squared = diff * diff;
        return squared->mean();
    }
};

class CrossEntropyLoss {
public:
    static std::shared_ptr<Tensor> compute(std::shared_ptr<Tensor> pred,
                                           std::shared_ptr<Tensor> target) {
        // ⚡ FIX: Flatten both tensors
        auto pred_flat = pred->flatten();
        auto target_flat = target->flatten();
        
        if (pred_flat->size() != target_flat->size()) {
            throw std::runtime_error("CrossEntropyLoss: prediction and target must have same total size");
        }
        
        // Simplified cross-entropy for binary classification
        // loss = -mean(target * log(pred) + (1-target) * log(1-pred))
        
        auto eps = 1e-7; // للاستقرار الرقمي
        
        // Clip predictions to avoid log(0)
        for (auto& p : pred_flat->data()) {
            p = std::max(eps, std::min(1.0 - eps, p));
        }
        
        auto log_pred = log_tensor(pred_flat);
        auto one_minus_pred = 1.0 * Tensor::ones(pred_flat->shape()) + (-1.0 * pred_flat);
        auto log_one_minus_pred = log_tensor(one_minus_pred);
        
        auto term1 = target_flat * log_pred;
        auto term2 = (1.0 * Tensor::ones(target_flat->shape()) + (-1.0 * target_flat)) * log_one_minus_pred;
        
        auto loss = term1 + term2;
        return (-1.0) * loss->mean();
    }

private:
    static std::shared_ptr<Tensor> log_tensor(std::shared_ptr<Tensor> t) {
        auto result = std::make_shared<Tensor>(t->shape(), t->requires_grad());
        
        for (size_t i = 0; i < t->size(); i++) {
            result->data()[i] = std::log(t->data()[i]);
        }
        
        result->inputs_ = {t};
        result->backward_fn_ = [t, result]() {
            if (t->requires_grad()) {
                for (size_t i = 0; i < t->size(); i++) {
                    t->grad()[i] += (1.0 / t->data()[i]) * result->grad()[i];
                }
            }
        };
        
        return result;
    }
};

// ==================== Example Usage ====================

void test_neural_network() {
    std::cout << "\n=== Testing Neural Network ===" << std::endl;
    
    // Create a simple network: 2 -> 4 -> 1
    Sequential model("simple_net");
    model.add(std::make_shared<Linear>(2, 4, "layer1"));
    model.add(std::make_shared<ReLU>("relu1"));
    model.add(std::make_shared<Linear>(4, 1, "layer2"));
    model.add(std::make_shared<Sigmoid>("sigmoid"));
    
    // XOR problem
    std::vector<std::vector<double>> X = {
        {0, 0}, {0, 1}, {1, 0}, {1, 1}
    };
    std::vector<double> y = {0, 1, 1, 0};
    
    std::cout << "\nTraining on XOR problem..." << std::endl;
    
    // Training loop
    double learning_rate = 0.1;
    int epochs = 1000;
    
    for (int epoch = 0; epoch < epochs; epoch++) {
        double total_loss = 0.0;
        
        for (size_t i = 0; i < X.size(); i++) {
            // Forward pass
            auto input = std::make_shared<Tensor>(X[i], Shape{2}, true);
            auto pred = model.forward(input);
            
            auto target = std::make_shared<Tensor>(
                std::vector<double>{y[i]}, Shape{1}, false
            );
            
            // Compute loss
            auto loss = MSELoss::compute(pred, target);
            total_loss += loss->data()[0];
            
            // Backward pass
            model.zero_grad();
            loss->backward();
            
            // Update parameters
            for (auto& param : model.parameters()) {
                for (size_t j = 0; j < param->size(); j++) {
                    param->data()[j] -= learning_rate * param->grad()[j];
                }
            }
        }
        
        if (epoch % 100 == 0) {
            std::cout << "Epoch " << epoch << " | Loss: " 
                      << (total_loss / X.size()) << std::endl;
        }
    }
    
    // Test
    std::cout << "\nTesting:" << std::endl;
    for (size_t i = 0; i < X.size(); i++) {
        auto input = std::make_shared<Tensor>(X[i], Shape{2}, false);
        auto pred = model.forward(input);
        std::cout << "Input: [" << X[i][0] << ", " << X[i][1] << "] "
                  << "→ Pred: " << pred->data()[0] 
                  << " (Expected: " << y[i] << ")" << std::endl;
    }
}

} // namespace nn
} // namespace darv

#endif // DARV_NN_LAYERS_HPP