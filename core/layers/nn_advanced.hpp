#ifndef DARV_NN_ADVANCED_HPP
#define DARV_NN_ADVANCED_HPP

#include "nn_layers.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace darv {
namespace nn {

// ==================== Dropout Layer ====================

class Dropout : public Layer {
private:
    double drop_rate_;
    bool training_;
    std::vector<bool> mask_;

public:
    Dropout(double drop_rate, const std::string& name = "dropout")
        : Layer(name), drop_rate_(drop_rate), training_(true) {}
    
    void set_training(bool mode) { training_ = mode; }
    
    std::shared_ptr<Tensor> forward(std::shared_ptr<Tensor> input) override {
        if (!training_) {
            return input;  // No dropout during inference
        }
        
        // Create dropout mask
        mask_.resize(input->size());
        auto result = std::make_shared<Tensor>(input->shape(), input->requires_grad());
        
        for (size_t i = 0; i < input->size(); i++) {
            double rand_val = static_cast<double>(rand()) / RAND_MAX;
            mask_[i] = (rand_val > drop_rate_);
            
            // Scale by 1/(1-p) to maintain expected value
            result->data()[i] = mask_[i] ? 
                input->data()[i] / (1.0 - drop_rate_) : 0.0;
        }
        
        result->inputs_ = {input};
        result->backward_fn_ = [this, input, result]() {
            if (input->requires_grad()) {
                for (size_t i = 0; i < input->size(); i++) {
                    input->grad()[i] += mask_[i] ? 
                        result->grad()[i] / (1.0 - drop_rate_) : 0.0;
                }
            }
        };
        
        return result;
    }
};

// ==================== Batch Normalization ====================

class BatchNorm : public Layer {
private:
    size_t num_features_;
    double eps_;
    double momentum_;
    
    std::shared_ptr<Tensor> gamma_;  // Scale
    std::shared_ptr<Tensor> beta_;   // Shift
    std::shared_ptr<Tensor> running_mean_;
    std::shared_ptr<Tensor> running_var_;
    
    bool training_;

public:
    BatchNorm(size_t num_features, double eps = 1e-5, double momentum = 0.1,
              const std::string& name = "batchnorm")
        : Layer(name), num_features_(num_features), 
          eps_(eps), momentum_(momentum), training_(true) {
        
        gamma_ = Tensor::ones(Shape{num_features}, true);
        beta_ = Tensor::zeros(Shape{num_features}, true);
        
        running_mean_ = Tensor::zeros(Shape{num_features}, false);
        running_var_ = Tensor::ones(Shape{num_features}, false);
        
        gamma_->set_name(name + ".gamma");
        beta_->set_name(name + ".beta");
        
        parameters_.push_back(gamma_);
        parameters_.push_back(beta_);
    }
    
    void set_training(bool mode) { training_ = mode; }
    
    std::shared_ptr<Tensor> forward(std::shared_ptr<Tensor> input) override {
        // Simplified batch norm for 1D/2D tensors
        auto shape = input->shape();
        size_t batch_size = (shape.size() == 2) ? shape[0] : 1;
        
        if (training_) {
            // Compute batch statistics
            std::vector<double> mean(num_features_, 0.0);
            std::vector<double> var(num_features_, 0.0);
            
            // Calculate mean
            for (size_t b = 0; b < batch_size; b++) {
                for (size_t f = 0; f < num_features_; f++) {
                    size_t idx = b * num_features_ + f;
                    mean[f] += input->data()[idx];
                }
            }
            for (auto& m : mean) m /= batch_size;
            
            // Calculate variance
            for (size_t b = 0; b < batch_size; b++) {
                for (size_t f = 0; f < num_features_; f++) {
                    size_t idx = b * num_features_ + f;
                    double diff = input->data()[idx] - mean[f];
                    var[f] += diff * diff;
                }
            }
            for (auto& v : var) v /= batch_size;
            
            // Update running statistics
            for (size_t f = 0; f < num_features_; f++) {
                running_mean_->data()[f] = 
                    (1 - momentum_) * running_mean_->data()[f] + momentum_ * mean[f];
                running_var_->data()[f] = 
                    (1 - momentum_) * running_var_->data()[f] + momentum_ * var[f];
            }
            
            // Normalize
            auto result = std::make_shared<Tensor>(input->shape(), input->requires_grad());
            for (size_t b = 0; b < batch_size; b++) {
                for (size_t f = 0; f < num_features_; f++) {
                    size_t idx = b * num_features_ + f;
                    double normalized = (input->data()[idx] - mean[f]) / 
                                       std::sqrt(var[f] + eps_);
                    result->data()[idx] = gamma_->data()[f] * normalized + beta_->data()[f];
                }
            }
            
            return result;
        } else {
            // Use running statistics
            auto result = std::make_shared<Tensor>(input->shape(), input->requires_grad());
            for (size_t b = 0; b < batch_size; b++) {
                for (size_t f = 0; f < num_features_; f++) {
                    size_t idx = b * num_features_ + f;
                    double normalized = (input->data()[idx] - running_mean_->data()[f]) / 
                                       std::sqrt(running_var_->data()[f] + eps_);
                    result->data()[idx] = gamma_->data()[f] * normalized + beta_->data()[f];
                }
            }
            return result;
        }
    }
};

// ==================== Model Serialization ====================

class ModelSerializer {
public:
    // Save model to file
    static bool save(Sequential& model, const std::string& filepath) {
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filepath << std::endl;
            return false;
        }
        
        auto params = model.parameters();
        
        // Write number of parameters
        size_t num_params = params.size();
        file.write(reinterpret_cast<const char*>(&num_params), sizeof(num_params));
        
        // Write each parameter
        for (auto& param : params) {
            // Write shape
            auto shape = param->shape();
            size_t shape_size = shape.size();
            file.write(reinterpret_cast<const char*>(&shape_size), sizeof(shape_size));
            file.write(reinterpret_cast<const char*>(shape.data()), 
                      shape_size * sizeof(size_t));
            
            // Write data
            size_t data_size = param->size();
            file.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
            file.write(reinterpret_cast<const char*>(param->data().data()), 
                      data_size * sizeof(double));
        }
        
        file.close();
        std::cout << "Model saved to: " << filepath << std::endl;
        return true;
    }
    
    // Load model from file
    static bool load(Sequential& model, const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << filepath << std::endl;
            return false;
        }
        
        auto params = model.parameters();
        
        // Read number of parameters
        size_t num_params;
        file.read(reinterpret_cast<char*>(&num_params), sizeof(num_params));
        
        if (num_params != params.size()) {
            std::cerr << "Parameter count mismatch!" << std::endl;
            file.close();
            return false;
        }
        
        // Read each parameter
        for (auto& param : params) {
            // Read shape
            size_t shape_size;
            file.read(reinterpret_cast<char*>(&shape_size), sizeof(shape_size));
            
            std::vector<size_t> shape(shape_size);
            file.read(reinterpret_cast<char*>(shape.data()), 
                     shape_size * sizeof(size_t));
            
            // Verify shape matches
            if (shape != param->shape()) {
                std::cerr << "Shape mismatch!" << std::endl;
                file.close();
                return false;
            }
            
            // Read data
            size_t data_size;
            file.read(reinterpret_cast<char*>(&data_size), sizeof(data_size));
            
            std::vector<double> data(data_size);
            file.read(reinterpret_cast<char*>(data.data()), 
                     data_size * sizeof(double));
            
            // Copy data to parameter
            for (size_t i = 0; i < data_size; i++) {
                param->data()[i] = data[i];
            }
        }
        
        file.close();
        std::cout << "Model loaded from: " << filepath << std::endl;
        return true;
    }
};

// ==================== Data Loader for Batches ====================

class DataLoader {
private:
    std::vector<std::vector<double>> X_;
    std::vector<double> y_;
    size_t batch_size_;
    bool shuffle_;
    std::vector<size_t> indices_;
    size_t current_idx_;

public:
    DataLoader(const std::vector<std::vector<double>>& X,
               const std::vector<double>& y,
               size_t batch_size,
               bool shuffle = true)
        : X_(X), y_(y), batch_size_(batch_size), 
          shuffle_(shuffle), current_idx_(0) {
        
        // Initialize indices
        indices_.resize(X_.size());
        for (size_t i = 0; i < X_.size(); i++) {
            indices_[i] = i;
        }
        
        if (shuffle_) {
            shuffle_indices();
        }
    }
    
    void shuffle_indices() {
        for (size_t i = indices_.size() - 1; i > 0; i--) {
            size_t j = rand() % (i + 1);
            std::swap(indices_[i], indices_[j]);
        }
    }
    
    bool has_next() const {
        return current_idx_ < X_.size();
    }
    
    std::pair<std::shared_ptr<Tensor>, std::shared_ptr<Tensor>> next_batch() {
        size_t batch_end = std::min(current_idx_ + batch_size_, X_.size());
        size_t actual_batch_size = batch_end - current_idx_;
        
        // Prepare batch data
        size_t feature_size = X_[0].size();
        std::vector<double> batch_X(actual_batch_size * feature_size);
        std::vector<double> batch_y(actual_batch_size);
        
        for (size_t i = 0; i < actual_batch_size; i++) {
            size_t idx = indices_[current_idx_ + i];
            
            // Copy features
            for (size_t j = 0; j < feature_size; j++) {
                batch_X[i * feature_size + j] = X_[idx][j];
            }
            
            // Copy label
            batch_y[i] = y_[idx];
        }
        
        current_idx_ = batch_end;
        
        // Create tensors
        auto X_tensor = std::make_shared<Tensor>(
            batch_X, Shape{actual_batch_size, feature_size}, true
        );
        auto y_tensor = std::make_shared<Tensor>(
            batch_y, Shape{actual_batch_size}, false
        );
        
        return {X_tensor, y_tensor};
    }
    
    void reset() {
        current_idx_ = 0;
        if (shuffle_) {
            shuffle_indices();
        }
    }
    
    size_t num_batches() const {
        return (X_.size() + batch_size_ - 1) / batch_size_;
    }
};

// ==================== Training Utilities ====================

class Trainer {
public:
    struct TrainingConfig {
        int epochs = 100;
        double learning_rate = 0.01;
        size_t batch_size = 32;
        bool verbose = true;
        int print_every = 10;
        std::string save_path = "";
    };
    
    struct TrainingHistory {
        std::vector<double> train_losses;
        std::vector<double> val_losses;
        std::vector<double> accuracies;
    };
    
    static TrainingHistory train(
        Sequential& model,
        const std::vector<std::vector<double>>& X_train,
        const std::vector<double>& y_train,
        const std::vector<std::vector<double>>& X_val,
        const std::vector<double>& y_val,
        const TrainingConfig& config
    ) {
        TrainingHistory history;
        
        DataLoader train_loader(X_train, y_train, config.batch_size, true);
        
        if (config.verbose) {
            std::cout << "\n=== Training Started ===" << std::endl;
            std::cout << "Epochs: " << config.epochs << std::endl;
            std::cout << "Batch size: " << config.batch_size << std::endl;
            std::cout << "Learning rate: " << config.learning_rate << std::endl;
            std::cout << "Training samples: " << X_train.size() << std::endl;
            std::cout << "Validation samples: " << X_val.size() << "\n" << std::endl;
        }
        
        for (int epoch = 0; epoch < config.epochs; epoch++) {
            double epoch_loss = 0.0;
            int num_batches = 0;
            
            train_loader.reset();
            
            while (train_loader.has_next()) {
                auto [X_batch, y_batch] = train_loader.next_batch();
                
                // Forward pass
                auto pred = model.forward(X_batch);
                
                // Reshape prediction if needed
                if (pred->shape().size() == 2 && pred->shape()[1] == 1) {
                    pred = pred->reshape(Shape{pred->shape()[0]});
                }
                
                // Compute loss
                auto loss = MSELoss::compute(pred, y_batch);
                epoch_loss += loss->data()[0];
                
                // Backward pass
                model.zero_grad();
                loss->backward();
                
                // Update parameters
                for (auto& param : model.parameters()) {
                    for (size_t i = 0; i < param->size(); i++) {
                        param->data()[i] -= config.learning_rate * param->grad()[i];
                    }
                }
                
                num_batches++;
            }
            
            double avg_loss = epoch_loss / num_batches;
            history.train_losses.push_back(avg_loss);
            
            // Validation
            if (!X_val.empty()) {
                double val_loss = evaluate(model, X_val, y_val);
                history.val_losses.push_back(val_loss);
                
                if (config.verbose && epoch % config.print_every == 0) {
                    std::cout << "Epoch " << std::setw(4) << epoch 
                              << " | Train Loss: " << std::fixed << std::setprecision(6) << avg_loss
                              << " | Val Loss: " << val_loss << std::endl;
                }
            } else {
                if (config.verbose && epoch % config.print_every == 0) {
                    std::cout << "Epoch " << std::setw(4) << epoch 
                              << " | Loss: " << std::fixed << std::setprecision(6) << avg_loss << std::endl;
                }
            }
        }
        
        if (config.verbose) {
            std::cout << "\n=== Training Completed ===" << std::endl;
        }
        
        // Save model if path provided
        if (!config.save_path.empty()) {
            ModelSerializer::save(model, config.save_path);
        }
        
        return history;
    }
    
    static double evaluate(Sequential& model,
                          const std::vector<std::vector<double>>& X,
                          const std::vector<double>& y) {
        double total_loss = 0.0;
        
        for (size_t i = 0; i < X.size(); i++) {
            auto input = std::make_shared<Tensor>(X[i], Shape{X[i].size()}, false);
            auto pred = model.forward(input);
            
            auto target = std::make_shared<Tensor>(
                std::vector<double>{y[i]}, Shape{1}, false
            );
            
            auto loss = MSELoss::compute(pred, target);
            total_loss += loss->data()[0];
        }
        
        return total_loss / X.size();
    }
};

} // namespace nn
} // namespace darv

#endif // DARV_NN_ADVANCED_HPP