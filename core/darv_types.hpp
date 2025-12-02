#ifndef DARV_TYPES_HPP
#define DARV_TYPES_HPP

#include <string>
#include <vector>
#include <chrono>

namespace darv {

// ==================== Project Configuration ====================
struct ProjectConfig {
    std::string name;
    std::string root_path;
    std::vector<std::string> build_commands;
    std::vector<std::string> run_commands;
    int max_cycles = 10;
    double convergence_threshold = 1.0;
};

// ==================== Execution Result ====================
struct ExecutionResult {
    bool success = false;
    int exit_code = 0;
    double execution_time_ms = 0.0;
    std::string stdout_output;
    std::string stderr_output;
    std::chrono::system_clock::time_point timestamp;
};

// ==================== Performance Metrics ====================
struct PerformanceMetrics {
    double avg_execution_time = 0.0;
    double min_execution_time = 0.0;
    double max_execution_time = 0.0;
    size_t error_count = 0;
    size_t warning_count = 0;
    size_t success_count = 0;
};

// ==================== Quality Evaluation ====================
struct QualityEvaluation {
    double overall_score = 0.0;
    PerformanceMetrics metrics;
    std::vector<std::string> issues;
    std::vector<std::string> suggestions;
    bool needs_improvement = false;
};

// ==================== Improvement Suggestion ====================
struct Improvement {
    std::string description;
    std::string target_file;
    std::string patch_content;
    double expected_impact = 0.0; // 0-1
    int priority = 0; // 1-10
};

// ==================== Cycle Record ====================
struct CycleRecord {
    int cycle_number = 0;
    std::chrono::system_clock::time_point timestamp;
    ExecutionResult execution;
    QualityEvaluation evaluation;
    std::vector<Improvement> improvements;
    bool applied_improvements = false;
    std::string notes;
};

} // namespace darv

#endif // DARV_TYPES_HPP