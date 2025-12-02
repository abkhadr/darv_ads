#ifndef DARV_PATH_B_HPP
#define DARV_PATH_B_HPP

#include "path_types.hpp"
#include "../darv_evaluator.hpp"
#include "../darv_improver.hpp"
#include <memory>
#include <algorithm>

namespace darv {
namespace dual_path {

// ==================== Path-B: Symbolic/Rule-based Evaluator ====================
// المسار الرمزي - يستخدم قواعد محددة ومنطق واضح
class PathB {
private:
    Evaluator rule_evaluator_;
    Improver rule_improver_;
    
    // Rule weights (can be adjusted based on experience)
    struct RuleWeights {
        double error_weight = 20.0;
        double warning_weight = 5.0;
        double performance_weight = 0.01;
        double complexity_weight = 0.5;
    } weights_;
    
    size_t num_evaluations_ = 0;

public:
    PathB() = default;
    
    // ==================== Evaluation ====================
    PathEvaluation evaluate(const CodeFeatures& features, const ExecutionResult& exec_result) {
        auto start = std::chrono::high_resolution_clock::now();
        
        PathEvaluation eval;
        eval.path_name = "Path-B (Symbolic)";
        eval.method_used = "symbolic";
        eval.confidence = 0.9; // Rule-based systems have high confidence
        
        // Use original evaluator
        QualityEvaluation quality_eval = rule_evaluator_.evaluate(exec_result);
        
        // Convert to PathEvaluation
        eval.quality_score = quality_eval.overall_score;
        eval.issues = quality_eval.issues;
        eval.suggestions = quality_eval.suggestions;
        
        // Add rule-based analysis
        analyze_with_rules(features, eval);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        eval.inference_time_ms = elapsed.count();
        
        num_evaluations_++;
        
        return eval;
    }
    
    // ==================== Rule-based Analysis ====================
    void analyze_with_rules(const CodeFeatures& features, PathEvaluation& eval) {
        // Rule 1: Complexity check
        if (features.cyclomatic_complexity > 50) {
            eval.issues.push_back("High cyclomatic complexity: " + 
                                 std::to_string(features.cyclomatic_complexity));
            eval.suggestions.push_back("Refactor complex functions");
            eval.quality_score -= weights_.complexity_weight * 
                                 (features.cyclomatic_complexity - 50);
        }
        
        // Rule 2: Performance check
        if (features.execution_time_ms > 2000.0) {
            eval.issues.push_back("Slow execution: " + 
                                 std::to_string(features.execution_time_ms) + "ms");
            eval.suggestions.push_back("Profile and optimize hotspots");
            eval.quality_score -= weights_.performance_weight * 
                                 (features.execution_time_ms - 2000.0);
        }
        
        // Rule 3: Error check
        if (features.compile_errors > 0) {
            eval.issues.push_back("Compilation errors: " + 
                                 std::to_string(features.compile_errors));
            eval.suggestions.push_back("Fix compilation errors first");
            eval.quality_score -= weights_.error_weight * features.compile_errors;
        }
        
        // Rule 4: Test coverage
        if (features.code_coverage < 0.7) {
            eval.issues.push_back("Low code coverage: " + 
                                 std::to_string(features.code_coverage * 100) + "%");
            eval.suggestions.push_back("Increase test coverage");
            eval.quality_score -= 10.0 * (0.7 - features.code_coverage);
        }
        
        // Rule 5: Memory usage
        if (features.memory_usage_kb > 100000) { // > 100MB
            eval.issues.push_back("High memory usage: " + 
                                 std::to_string(features.memory_usage_kb / 1024) + "MB");
            eval.suggestions.push_back("Investigate memory leaks");
            eval.quality_score -= 5.0;
        }
        
        // Ensure score is in [0, 100]
        eval.quality_score = std::max(0.0, std::min(100.0, eval.quality_score));
    }
    
    // ==================== Improvement Generation ====================
    std::vector<PathImprovement> suggest_improvements(const CodeFeatures& features,
                                                      const QualityEvaluation& quality_eval,
                                                      const ProjectConfig& config) {
        std::vector<PathImprovement> improvements;
        
        // Use original improver
        auto rule_improvements = rule_improver_.generate_improvements(quality_eval, config);
        
        // Convert to PathImprovement
        for (const auto& imp : rule_improvements) {
            PathImprovement path_imp;
            path_imp.path_name = "Path-B";
            path_imp.description = imp.description;
            path_imp.target_file = imp.target_file;
            path_imp.patch_content = imp.patch_content;
            path_imp.expected_impact = imp.expected_impact;
            path_imp.confidence = 0.85; // High confidence for rule-based
            path_imp.priority = imp.priority;
            path_imp.reasoning = "Rule-based analysis";
            
            improvements.push_back(path_imp);
        }
        
        // Add custom rule-based improvements
        add_rule_based_improvements(features, improvements);
        
        // Sort by priority
        std::sort(improvements.begin(), improvements.end(),
                 [](const auto& a, const auto& b) { return a.priority > b.priority; });
        
        return improvements;
    }
    
    // ==================== Custom Rule-based Improvements ====================
    void add_rule_based_improvements(const CodeFeatures& features,
                                     std::vector<PathImprovement>& improvements) {
        // Complexity improvement
        if (features.cyclomatic_complexity > 30) {
            PathImprovement imp;
            imp.path_name = "Path-B";
            imp.description = "Reduce code complexity";
            imp.expected_impact = 0.6;
            imp.confidence = 0.9;
            imp.priority = 7;
            imp.reasoning = "High complexity detected: " + 
                           std::to_string(features.cyclomatic_complexity);
            improvements.push_back(imp);
        }
        
        // Performance improvement
        if (features.execution_time_ms > 1000.0) {
            PathImprovement imp;
            imp.path_name = "Path-B";
            imp.description = "Add optimization flags (-O3)";
            imp.target_file = "CMakeLists.txt";
            imp.patch_content = "set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -O3\")\n";
            imp.expected_impact = 0.7;
            imp.confidence = 0.95;
            imp.priority = 9;
            imp.reasoning = "Slow execution detected: " + 
                           std::to_string(features.execution_time_ms) + "ms";
            improvements.push_back(imp);
        }
        
        // Test coverage improvement
        if (features.code_coverage < 0.5) {
            PathImprovement imp;
            imp.path_name = "Path-B";
            imp.description = "Increase test coverage";
            imp.expected_impact = 0.4;
            imp.confidence = 0.8;
            imp.priority = 6;
            imp.reasoning = "Low test coverage: " + 
                           std::to_string(features.code_coverage * 100) + "%";
            improvements.push_back(imp);
        }
    }
    
    // ==================== Learning (Adjust Weights) ====================
    void learn_from_feedback(const CodeFeatures& features,
                            double actual_quality,
                            double predicted_quality) {
        // Adjust rule weights based on prediction error
        double error = std::abs(actual_quality - predicted_quality);
        
        // Simple weight adjustment (gradient descent style)
        double learning_rate = 0.01;
        
        if (error > 10.0) {
            // If large error, adjust weights
            if (features.compile_errors > 0 && actual_quality < predicted_quality) {
                weights_.error_weight += learning_rate * error;
            }
            if (features.execution_time_ms > 1000.0 && actual_quality < predicted_quality) {
                weights_.performance_weight += learning_rate * error / 100.0;
            }
        }
        
        std::cout << "[Path-B] Adjusted rule weights based on feedback" << std::endl;
    }
    
    // ==================== Stats ====================
    void print_stats() const {
        std::cout << "\n╔════════════════════════════════════════╗\n";
        std::cout << "║         Path-B Statistics              ║\n";
        std::cout << "╚════════════════════════════════════════╝\n";
        std::cout << "Total evaluations: " << num_evaluations_ << "\n";
        std::cout << "Rule weights:\n";
        std::cout << "  Error weight: " << weights_.error_weight << "\n";
        std::cout << "  Warning weight: " << weights_.warning_weight << "\n";
        std::cout << "  Performance weight: " << weights_.performance_weight << "\n";
        std::cout << "  Complexity weight: " << weights_.complexity_weight << "\n";
    }
    
    // Get rule weights for saving
    const RuleWeights& get_weights() const { return weights_; }
    void set_weights(const RuleWeights& weights) { weights_ = weights; }
};

} // namespace dual_path
} // namespace darv

#endif // DARV_PATH_B_HPP