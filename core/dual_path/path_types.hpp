#ifndef DARV_PATH_TYPES_HPP
#define DARV_PATH_TYPES_HPP

#include "../darv_types.hpp"
#include "../darv_tensor.hpp"
#include <vector>
#include <string>
#include <memory>

namespace darv {
namespace dual_path {

// ==================== Code Features ====================
// تمثيل ميزات الكود للشبكة العصبية
struct CodeFeatures {
    // Static features
    size_t lines_of_code;
    size_t num_functions;
    size_t num_classes;
    size_t cyclomatic_complexity;
    
    // Dynamic features (from execution)
    double execution_time_ms;
    size_t memory_usage_kb;
    int exit_code;
    
    // Error features
    size_t compile_errors;
    size_t runtime_errors;
    size_t warnings;
    
    // Quality features
    double code_coverage;
    size_t test_passed;
    size_t test_failed;
    
    // Convert to tensor for neural network
    std::shared_ptr<autograd::Tensor> to_tensor() const {
        std::vector<double> features = {
            static_cast<double>(lines_of_code) / 1000.0,      // Normalize
            static_cast<double>(num_functions) / 100.0,
            static_cast<double>(num_classes) / 50.0,
            static_cast<double>(cyclomatic_complexity) / 50.0,
            execution_time_ms / 1000.0,
            static_cast<double>(memory_usage_kb) / 10000.0,
            static_cast<double>(exit_code),
            static_cast<double>(compile_errors),
            static_cast<double>(runtime_errors),
            static_cast<double>(warnings) / 10.0,
            code_coverage,
            static_cast<double>(test_passed) / 100.0,
            static_cast<double>(test_failed) / 10.0
        };
        
        return std::make_shared<autograd::Tensor>(
            features, 
            autograd::Shape{features.size()}, 
            false
        );
    }
    
    static size_t feature_size() { return 13; }
};

// ==================== Path Evaluation ====================
// نتيجة تقييم من أحد الـ Paths
struct PathEvaluation {
    std::string path_name;           // "Path-A" or "Path-B"
    double confidence;               // 0-1: ثقة Path في التقييم
    double quality_score;            // 0-100: تقييم الجودة
    std::vector<std::string> issues; // المشاكل المكتشفة
    std::vector<std::string> suggestions; // اقتراحات التحسين
    
    // Metadata
    double inference_time_ms;
    std::string method_used;         // "neural", "symbolic", "hybrid"
};

// ==================== Path Improvement ====================
// تحسين مقترح من أحد الـ Paths
struct PathImprovement {
    std::string path_name;
    std::string description;
    std::string target_file;
    std::string patch_content;
    
    double expected_impact;          // 0-1: التأثير المتوقع
    double confidence;               // 0-1: ثقة Path في التحسين
    int priority;                    // 1-10
    
    std::string reasoning;           // سبب التحسين
};

// ==================== Knowledge Entry ====================
// معلومة واحدة في قاعدة المعرفة
struct KnowledgeEntry {
    // Input features
    CodeFeatures features;
    
    // Evaluation results
    PathEvaluation path_a_eval;
    PathEvaluation path_b_eval;
    
    // Ground truth (from actual execution)
    double actual_quality;
    bool improvement_worked;
    
    // Meta learning
    double path_a_error;             // |predicted - actual|
    double path_b_error;
    
    // Timestamp
    std::chrono::system_clock::time_point timestamp;
    int cycle_number;
};

// ==================== Dual Path Decision ====================
// قرار نهائي يجمع Path-A و Path-B
struct DualPathDecision {
    PathEvaluation final_evaluation;
    std::vector<PathImprovement> improvements;
    
    // Consensus info
    bool paths_agree;                // هل Path-A و Path-B متفقين؟
    double agreement_score;          // 0-1: درجة الاتفاق
    
    // Path weights (learned over time)
    double path_a_weight;            // وزن Path-A في القرار
    double path_b_weight;            // وزن Path-B في القرار
    
    // Decision strategy
    std::string strategy;            // "weighted_average", "path_a_dominant", "path_b_dominant"
    
    void print_summary() const {
        std::cout << "\n╔════════════════════════════════════════╗\n";
        std::cout << "║     Dual-Path Decision Summary         ║\n";
        std::cout << "╚════════════════════════════════════════╝\n";
        std::cout << "Quality Score: " << final_evaluation.quality_score << "/100\n";
        std::cout << "Paths Agree: " << (paths_agree ? "Yes" : "No") << "\n";
        std::cout << "Agreement: " << (agreement_score * 100) << "%\n";
        std::cout << "Path-A Weight: " << path_a_weight << "\n";
        std::cout << "Path-B Weight: " << path_b_weight << "\n";
        std::cout << "Strategy: " << strategy << "\n";
        std::cout << "Improvements: " << improvements.size() << "\n";
    }
};

// ==================== Training Sample ====================
// عينة تدريب للشبكة العصبية
struct TrainingSample {
    std::shared_ptr<autograd::Tensor> features;
    std::shared_ptr<autograd::Tensor> target;
    
    // Metadata
    double importance_weight;        // وزن العينة في التدريب
    int cycle_number;
};

} // namespace dual_path
} // namespace darv

#endif // DARV_PATH_TYPES_HPP