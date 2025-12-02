#ifndef DARV_DATASET_HPP
#define DARV_DATASET_HPP

#include "darv_tensor.hpp"
#include <vector>
#include <memory>
#include <random>
#include <algorithm>

namespace darv {
namespace autograd {

// Dataset: تخزين وإدارة البيانات
class Dataset {
protected:
    std::vector<std::shared_ptr<Tensor>> data_;
    std::vector<std::shared_ptr<Tensor>> labels_;
    size_t size_;

public:
    Dataset() : size_(0) {}
    
    Dataset(const std::vector<std::shared_ptr<Tensor>>& data,
            const std::vector<std::shared_ptr<Tensor>>& labels)
        : data_(data), labels_(labels) {
        
        if (data.size() != labels.size()) {
            throw std::runtime_error("Data and labels must have same size");
        }
        size_ = data.size();
    }
    
    // Get single sample
    std::pair<std::shared_ptr<Tensor>, std::shared_ptr<Tensor>> operator[](size_t idx) const {
        if (idx >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return {data_[idx], labels_[idx]};
    }
    
    size_t size() const { return size_; }
    
    // Add sample
    void add_sample(std::shared_ptr<Tensor> data, std::shared_ptr<Tensor> label) {
        data_.push_back(data);
        labels_.push_back(label);
        size_++;
    }
    
    // Get all data and labels
    const std::vector<std::shared_ptr<Tensor>>& data() const { return data_; }
    const std::vector<std::shared_ptr<Tensor>>& labels() const { return labels_; }
    
    // Shuffle dataset
    void shuffle() {
        std::vector<size_t> indices(size_);
        std::iota(indices.begin(), indices.end(), 0);
        
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(indices.begin(), indices.end(), g);
        
        std::vector<std::shared_ptr<Tensor>> shuffled_data;
        std::vector<std::shared_ptr<Tensor>> shuffled_labels;
        
        for (size_t idx : indices) {
            shuffled_data.push_back(data_[idx]);
            shuffled_labels.push_back(labels_[idx]);
        }
        
        data_ = shuffled_data;
        labels_ = shuffled_labels;
    }
    
    // Split dataset
    std::pair<Dataset, Dataset> train_test_split(double train_ratio = 0.8) {
        size_t train_size = static_cast<size_t>(size_ * train_ratio);
        
        Dataset train_set, test_set;
        
        for (size_t i = 0; i < size_; i++) {
            if (i < train_size) {
                train_set.add_sample(data_[i], labels_[i]);
            } else {
                test_set.add_sample(data_[i], labels_[i]);
            }
        }
        
        return {train_set, test_set};
    }
    
    // Print statistics
    void print_stats() const {
        std::cout << "Dataset Statistics:" << std::endl;
        std::cout << "  Size: " << size_ << std::endl;
        
        if (!data_.empty()) {
            auto& first_data = data_[0];
            std::cout << "  Data shape: [";
            for (size_t i = 0; i < first_data->shape().size(); i++) {
                std::cout << first_data->shape()[i];
                if (i < first_data->shape().size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }
        
        if (!labels_.empty()) {
            auto& first_label = labels_[0];
            std::cout << "  Label shape: [";
            for (size_t i = 0; i < first_label->shape().size(); i++) {
                std::cout << first_label->shape()[i];
                if (i < first_label->shape().size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }
    }
};

// DataLoader: تحميل البيانات في batches
class DataLoader {
private:
    Dataset dataset_;
    size_t batch_size_;
    bool shuffle_;
    size_t current_idx_;
    std::vector<size_t> indices_;

public:
    DataLoader(const Dataset& dataset, size_t batch_size = 32, bool shuffle = true)
        : dataset_(dataset), batch_size_(batch_size), shuffle_(shuffle), current_idx_(0) {
        
        // Initialize indices
        indices_.resize(dataset_.size());
        std::iota(indices_.begin(), indices_.end(), 0);
        
        if (shuffle_) {
            shuffle_indices();
        }
    }
    
    // Shuffle indices
    void shuffle_indices() {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(indices_.begin(), indices_.end(), g);
    }
    
    // Reset iterator
    void reset() {
        current_idx_ = 0;
        if (shuffle_) {
            shuffle_indices();
        }
    }
    
    // Check if more batches available
    bool has_next() const {
        return current_idx_ < dataset_.size();
    }
    
    // Get next batch
    std::pair<std::vector<std::shared_ptr<Tensor>>, 
              std::vector<std::shared_ptr<Tensor>>> next_batch() {
        
        if (!has_next()) {
            throw std::runtime_error("No more batches available");
        }
        
        size_t batch_start = current_idx_;
        size_t batch_end = std::min(current_idx_ + batch_size_, dataset_.size());
        
        std::vector<std::shared_ptr<Tensor>> batch_data;
        std::vector<std::shared_ptr<Tensor>> batch_labels;
        
        for (size_t i = batch_start; i < batch_end; i++) {
            size_t idx = indices_[i];
            auto [data, label] = dataset_[idx];
            batch_data.push_back(data);
            batch_labels.push_back(label);
        }
        
        current_idx_ = batch_end;
        
        return {batch_data, batch_labels};
    }
    
    // Get number of batches
    size_t num_batches() const {
        return (dataset_.size() + batch_size_ - 1) / batch_size_;
    }
    
    // Iterator interface
    class Iterator {
    private:
        DataLoader* loader_;
        bool end_;
        
    public:
        Iterator(DataLoader* loader, bool end = false) 
            : loader_(loader), end_(end) {}
        
        bool operator!=(const Iterator& other) const {
            return end_ != other.end_;
        }
        
        Iterator& operator++() {
            if (!loader_->has_next()) {
                end_ = true;
            }
            return *this;
        }
        
        std::pair<std::vector<std::shared_ptr<Tensor>>, 
                  std::vector<std::shared_ptr<Tensor>>> operator*() {
            return loader_->next_batch();
        }
    };
    
    Iterator begin() {
        reset();
        return Iterator(this, !has_next());
    }
    
    Iterator end() {
        return Iterator(this, true);
    }
};

// ==================== مساعدات لإنشاء Datasets ====================

// إنشاء dataset عشوائي للتجربة
Dataset create_random_dataset(size_t num_samples, const Shape& data_shape, 
                              const Shape& label_shape) {
    Dataset dataset;
    
    for (size_t i = 0; i < num_samples; i++) {
        auto data = Tensor::randn(data_shape, false);
        auto label = Tensor::randn(label_shape, false);
        dataset.add_sample(data, label);
    }
    
    return dataset;
}

// إنشاء dataset من vectors
Dataset create_dataset_from_vectors(
    const std::vector<std::vector<double>>& X,
    const std::vector<std::vector<double>>& y) {
    
    if (X.size() != y.size()) {
        throw std::runtime_error("X and y must have same size");
    }
    
    Dataset dataset;
    
    for (size_t i = 0; i < X.size(); i++) {
        auto data = std::make_shared<Tensor>(X[i], Shape{X[i].size()}, false);
        auto label = std::make_shared<Tensor>(y[i], Shape{y[i].size()}, false);
        dataset.add_sample(data, label);
    }
    
    return dataset;
}

// إنشاء dataset للـ classification
Dataset create_classification_dataset(size_t num_samples, size_t num_features, 
                                      size_t num_classes) {
    Dataset dataset;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1.0, 1.0);
    
    for (size_t i = 0; i < num_samples; i++) {
        // Generate random features
        std::vector<double> features(num_features);
        for (auto& f : features) {
            f = dis(gen);
        }
        auto data = std::make_shared<Tensor>(features, Shape{num_features}, false);
        
        // Generate one-hot label
        std::vector<double> label_vec(num_classes, 0.0);
        size_t class_idx = rand() % num_classes;
        label_vec[class_idx] = 1.0;
        auto label = std::make_shared<Tensor>(label_vec, Shape{num_classes}, false);
        
        dataset.add_sample(data, label);
    }
    
    return dataset;
}

// Normalize dataset
void normalize_dataset(Dataset& dataset) {
    if (dataset.size() == 0) return;
    
    size_t feature_size = dataset.data()[0]->size();
    
    // Calculate mean and std for each feature
    std::vector<double> means(feature_size, 0.0);
    std::vector<double> stds(feature_size, 0.0);
    
    // Calculate means
    for (const auto& sample : dataset.data()) {
        const auto& data = sample->data();
        for (size_t i = 0; i < feature_size; i++) {
            means[i] += data[i];
        }
    }
    for (auto& m : means) {
        m /= dataset.size();
    }
    
    // Calculate stds
    for (const auto& sample : dataset.data()) {
        const auto& data = sample->data();
        for (size_t i = 0; i < feature_size; i++) {
            double diff = data[i] - means[i];
            stds[i] += diff * diff;
        }
    }
    for (auto& s : stds) {
        s = std::sqrt(s / dataset.size());
        if (s < 1e-7) s = 1.0; // Avoid division by zero
    }
    
    // Normalize
    for (auto& sample : dataset.data()) {
        auto& data = sample->data();
        for (size_t i = 0; i < feature_size; i++) {
            data[i] = (data[i] - means[i]) / stds[i];
        }
    }
    
    std::cout << "Dataset normalized (mean=0, std=1)" << std::endl;
}

} // namespace autograd
} // namespace darv

#endif // DARV_DATASET_HPP