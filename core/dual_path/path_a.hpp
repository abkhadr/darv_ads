#ifndef DARV_PATH_A_HPP
#define DARV_PATH_A_HPP

#include "path_types.hpp"
#include "../layers/nn_layers.hpp"
#include "../layers/nn_advanced.hpp"
#include "../darv_optimizer.hpp"
#include <memory>

namespace darv {
namespace dual_path {

using namespace autograd;
using namespace nn;

// ==================== Path-A: Neural Evaluator ====================
// المسار العصبي - يتعلم من الخبرة
class PathA {
private:
    Sequential quality_model_;       // تقييم الجودة
    Sequential improvement_model_;   // اختيار التحسينات
    
    std::unique_ptr<Adam> optimizer_;
    
    // Training history
    std::vector<TrainingSample> training_buffer_;
    size_t buffer_capacity_ = 1000;
    
    // Performance tracking
    double avg_prediction_error_ = 0.0;
    size_t num_predictions_ = 0;
    
    bool is_trained_ = false;

public:
    PathA() {
        // Quality evaluation model: Features -> Quality Score
        quality_model_ = Sequential("path_a_quality");
        quality_model_.add(std::make_shared<Linear>(CodeFeatures::feature_size(), 32, "q_fc1"));
        quality_model_.add(std::make_shared<ReLU>("q_relu1"));
        quality_model_.add(std::make_shared<Dropout>(0.2, "q_dropout1"));
        quality_model_.add(std::make_shared<Linear>(32, 16, "q_fc2"));
        quality_model_.add(std::make_shared<ReLU>("q_relu2"));
        quality_model_.add(std::make_shared<Linear>(16, 1, "q_output"));
        quality_model_.add(std::make_shared<Sigmoid>("q_sigmoid"));
        
        // Improvement selection model
        improvement_model_ = Sequential("path_a_improvement");
        improvement_model_.add(std::make_shared<Linear>(CodeFeatures::feature_size(), 24, "i_fc1"));
        improvement_model_.add(std::make_shared<ReLU>("i_relu1"));
        improvement_model_.add(std::make_shared<Linear>(24, 12, "i_fc2"));
        improvement_model_.add(std::make_shared<ReLU>("i_relu2"));
        improvement_model_.add(std::make_shared<Linear>(12, 5, "i_output")); // 5 improvement types
        
        // Setup optimizer
        auto all_params = quality_model_.parameters();
        auto imp_params = improvement_model_.parameters();
        all_params.insert(all_params.end(), imp_params.begin(), imp_params.end());
        
        optimizer_ = std::make_unique<Adam>(all_params, 0.001);
    }
    
    // ==================== Evaluation ====================
    PathEvaluation evaluate(const CodeFeatures& features) {
        auto start = std::chrono::high_resolution_clock::now();
        
        PathEvaluation eval;
        eval.path_name = "Path-A (Neural)";
        eval.method_used = "neural";
        
        if (!is_trained_) {
            // If not trained yet, use heuristics
            eval.quality_score = 50.0;
            eval.confidence = 0.3;
            eval.suggestions.push_back("Path-A needs training data");
            
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            eval.inference_time_ms = elapsed.count();
            
            return eval;
        }
        
        // Neural inference
        auto input = features.to_tensor();
        auto quality_pred = quality_model_.forward(input);
        
        // Convert [0,1] to [0,100]
        double quality = quality_pred->data()[0] * 100.0;
        
        eval.quality_score = quality;
        eval.confidence = calculate_confidence(features);
        
        // Generate issues based on score
        if (quality < 50.0) {
            eval.issues.push_back("Low quality predicted by neural model");
        }
        if (features.compile_errors > 0) {
            eval.issues.push_back("Compilation errors detected");
        }
        if (features.execution_time_ms > 1000.0) {
            eval.issues.push_back("Slow execution time");
        }
        
        // Generate suggestions
        eval.suggestions = generate_suggestions(features);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        eval.inference_time_ms = elapsed.count();
        
        num_predictions_++;
        
        return eval;
    }
    
    // ==================== Improvement Generation ====================
    std::vector<PathImprovement> suggest_improvements(const CodeFeatures& features) {
        std::vector<PathImprovement> improvements;
        
        if (!is_trained_) {
            return improvements; // No suggestions without training
        }
        
        // Neural inference for improvement types
        auto input = features.to_tensor();
        auto imp_pred = improvement_model_.forward(input);
        
        // Top-k improvements
        std::vector<std::pair<size_t, double>> scores;
        for (size_t i = 0; i < imp_pred->size(); i++) {
            scores.push_back({i, imp_pred->data()[i]});
        }
        std::sort(scores.begin(), scores.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Create improvements for top 3
        std::vector<std::string> improvement_types = {
            "Add optimization flags",
            "Reduce complexity",
            "Improve error handling",
            "Add caching",
            "Parallel execution"
        };
        
        for (size_t i = 0; i < std::min(size_t(3), scores.size()); i++) {
            size_t idx = scores[i].first;
            double score = scores[i].second;
            
            if (score > 0.5) { // Threshold
                PathImprovement imp;
                imp.path_name = "Path-A";
                imp.description = improvement_types[idx];
                imp.expected_impact = score;
                imp.confidence = calculate_confidence(features);
                imp.priority = static_cast<int>((1.0 - i * 0.2) * 10);
                imp.reasoning = "Neural model suggests this improvement";
                
                improvements.push_back(imp);
            }
        }
        
        return improvements;
    }
    
    // ==================== Learning ====================
    void learn_from_feedback(const CodeFeatures& features,
                            double actual_quality,
                            bool improvement_worked) {
        // Create training sample
        TrainingSample sample;
        sample.features = features.to_tensor();
        sample.target = std::make_shared<Tensor>(
            std::vector<double>{actual_quality / 100.0}, // Normalize to [0,1]
            Shape{1},
            false
        );
        sample.importance_weight = 1.0;
        
        // Add to buffer
        training_buffer_.push_back(sample);
        
        // Keep buffer size limited
        if (training_buffer_.size() > buffer_capacity_) {
            training_buffer_.erase(training_buffer_.begin());
        }
        
        // Train if enough samples
        if (training_buffer_.size() >= 10) {
            train_step();
        }
    }
    
    // ==================== Training ====================
    void train_step(int epochs = 5) {
        if (training_buffer_.empty()) return;
        
        std::cout << "[Path-A] Training on " << training_buffer_.size() << " samples..." << std::endl;
        
        for (int epoch = 0; epoch < epochs; epoch++) {
            double total_loss = 0.0;
            
            for (const auto& sample : training_buffer_) {
                // Forward
                auto pred = quality_model_.forward(sample.features);
                auto loss = MSELoss::compute(pred, sample.target);
                
                total_loss += loss->data()[0];
                
                // Backward
                quality_model_.zero_grad();
                loss->backward();
                
                // Update
                optimizer_->step();
            }
            
            double avg_loss = total_loss / training_buffer_.size();
            
            if (epoch % 2 == 0) {
                std::cout << "  Epoch " << epoch << " | Loss: " << avg_loss << std::endl;
            }
        }
        
        is_trained_ = true;
        std::cout << "[Path-A] Training complete ✓" << std::endl;
    }
    
    // ==================== Utilities ====================
    double calculate_confidence(const CodeFeatures& features) const {
        // Confidence based on training samples and feature similarity
        if (!is_trained_ || training_buffer_.empty()) {
            return 0.3;
        }
        
        // Simple heuristic: more training = more confidence
        double base_confidence = std::min(0.95, 0.5 + training_buffer_.size() / 200.0);
        
        return base_confidence;
    }
    
    std::vector<std::string> generate_suggestions(const CodeFeatures& features) const {
        std::vector<std::string> suggestions;
        
        if (features.execution_time_ms > 1000.0) {
            suggestions.push_back("Consider optimization techniques");
        }
        if (features.cyclomatic_complexity > 30) {
            suggestions.push_back("Reduce code complexity");
        }
        if (features.warnings > 5) {
            suggestions.push_back("Fix compiler warnings");
        }
        
        return suggestions;
    }
    
    // Save/Load model
    bool save(const std::string& path) {
        return ModelSerializer::save(quality_model_, path + "_quality.bin") &&
               ModelSerializer::save(improvement_model_, path + "_improvement.bin");
    }
    
    bool load(const std::string& path) {
        bool success = ModelSerializer::load(quality_model_, path + "_quality.bin") &&
                      ModelSerializer::load(improvement_model_, path + "_improvement.bin");
        if (success) {
            is_trained_ = true;
        }
        return success;
    }
    
    // Stats
    void print_stats() const {
        std::cout << "\n╔════════════════════════════════════════╗\n";
        std::cout << "║         Path-A Statistics              ║\n";
        std::cout << "╚════════════════════════════════════════╝\n";
        std::cout << "Training samples: " << training_buffer_.size() << "\n";
        std::cout << "Total predictions: " << num_predictions_ << "\n";
        std::cout << "Is trained: " << (is_trained_ ? "Yes" : "No") << "\n";
        std::cout << "Avg error: " << avg_prediction_error_ << "\n";
    }
};

} // namespace dual_path
} // namespace darv

#endif // DARV_PATH_A_HPP