#ifndef DARV_DUAL_CYCLE_HPP
#define DARV_DUAL_CYCLE_HPP

#include "path_a.hpp"
#include "path_b.hpp"
#include "knowledge_base.hpp"
#include "../darv_executor.hpp"
#include <memory>
#include <thread>

namespace darv {
namespace dual_path {

// ==================== Dual-Path DARV Cycle ====================
// نظام مدمج يجمع Path-A (Neural) و Path-B (Symbolic)
class DualCycle {
private:
    std::unique_ptr<PathA> path_a_;
    std::unique_ptr<PathB> path_b_;
    std::unique_ptr<KnowledgeBase> knowledge_;
    Executor executor_;
    
    ProjectConfig config_;
    std::string memory_path_;
    
    // Current weights
    double path_a_weight_ = 0.5;
    double path_b_weight_ = 0.5;
    
    // History
    std::vector<DualPathDecision> decision_history_;
    int current_cycle_ = 0;

public:
    DualCycle(const ProjectConfig& config, const std::string& memory_path)
        : config_(config), memory_path_(memory_path) {
        
        path_a_ = std::make_unique<PathA>();
        path_b_ = std::make_unique<PathB>();
        knowledge_ = std::make_unique<KnowledgeBase>(memory_path + "/knowledge_base");
        
        // Try to load previous knowledge
        knowledge_->load();
        
        // Try to load Path-A model
        path_a_->load(memory_path + "/path_a_model");
        
        std::cout << "\n╔════════════════════════════════════════╗\n";
        std::cout << "║      Dual-Path DARV System Initialized ║\n";
        std::cout << "╚════════════════════════════════════════╝\n";
    }
    
    // ==================== Extract Features ====================
    CodeFeatures extract_features(const ExecutionResult& exec_result) {
        CodeFeatures features{};
        
        // Extract from execution result
        features.execution_time_ms = exec_result.execution_time_ms;
        features.exit_code = exec_result.exit_code;
        
        // Count errors and warnings
        features.compile_errors = std::count(
            exec_result.stderr_output.begin(),
            exec_result.stderr_output.end(), ':') / 3; // Rough estimate
        
        features.warnings = std::count_if(
            exec_result.stderr_output.begin(),
            exec_result.stderr_output.end(),
            [](char c) { return c == 'w' || c == 'W'; }) / 10;
        
        // TODO: Extract more features from code analysis
        features.lines_of_code = 500;  // Placeholder
        features.num_functions = 20;
        features.cyclomatic_complexity = 15;
        features.code_coverage = 0.6;
        
        return features;
    }
    
    // ==================== Run Single Cycle ====================
    DualPathDecision run_single_cycle() {
        current_cycle_++;
        
        std::cout << "\n╔════════════════════════════════════════╗\n";
        std::cout << "║   Dual-Path Cycle #" << current_cycle_ << "                     ║\n";
        std::cout << "╚════════════════════════════════════════╝\n";
        
        // Step 1: Build & Execute
        std::cout << "\n► Step 1/5: Build & Execute\n";
        ExecutionResult build_result = executor_.build_project(config_);
        
        if (!build_result.success) {
            std::cout << "Build failed. Skipping evaluation.\n";
            return create_failed_decision();
        }
        
        ExecutionResult run_result = executor_.run_project(config_);
        
        // Step 2: Extract Features
        std::cout << "\n► Step 2/5: Feature Extraction\n";
        CodeFeatures features = extract_features(run_result);
        
        // Step 3: Path-A Evaluation (Neural)
        std::cout << "\n► Step 3/5: Path-A Evaluation (Neural)\n";
        PathEvaluation path_a_eval = path_a_->evaluate(features);
        std::cout << "  Quality: " << path_a_eval.quality_score 
                  << " (confidence: " << path_a_eval.confidence << ")\n";
        
        // Step 4: Path-B Evaluation (Symbolic)
        std::cout << "\n► Step 4/5: Path-B Evaluation (Symbolic)\n";
        PathEvaluation path_b_eval = path_b_->evaluate(features, run_result);
        std::cout << "  Quality: " << path_b_eval.quality_score 
                  << " (confidence: " << path_b_eval.confidence << ")\n";
        
        // Step 5: Combine & Decide
        std::cout << "\n► Step 5/5: Dual-Path Decision\n";
        DualPathDecision decision = combine_evaluations(
            path_a_eval, path_b_eval, features, run_result
        );
        
        decision.print_summary();
        
        // Store knowledge
        store_knowledge_entry(features, path_a_eval, path_b_eval, decision);
        
        // Learn from this cycle
        double actual_quality = decision.final_evaluation.quality_score;
        path_a_->learn_from_feedback(features, actual_quality, true);
        path_b_->learn_from_feedback(features, actual_quality, path_b_eval.quality_score);
        
        // Update weights
        update_weights();
        
        decision_history_.push_back(decision);
        
        return decision;
    }
    
    // ==================== Combine Evaluations ====================
    DualPathDecision combine_evaluations(const PathEvaluation& path_a_eval,
                                         const PathEvaluation& path_b_eval,
                                         const CodeFeatures& features,
                                         const ExecutionResult& exec_result) {
        DualPathDecision decision;
        
        // Calculate agreement
        double score_diff = std::abs(path_a_eval.quality_score - path_b_eval.quality_score);
        decision.agreement_score = 1.0 - (score_diff / 100.0);
        decision.paths_agree = (score_diff < 15.0);
        
        // Set current weights
        decision.path_a_weight = path_a_weight_;
        decision.path_b_weight = path_b_weight_;
        
        // Determine strategy
        if (decision.paths_agree) {
            decision.strategy = "weighted_average";
        } else if (path_a_weight_ > 0.7) {
            decision.strategy = "path_a_dominant";
        } else if (path_b_weight_ > 0.7) {
            decision.strategy = "path_b_dominant";
        } else {
            decision.strategy = "weighted_average";
        }
        
        // Combine evaluations
        decision.final_evaluation = path_a_eval; // Start with path_a
        
        if (decision.strategy == "weighted_average") {
            // Weighted average of scores
            decision.final_evaluation.quality_score = 
                path_a_weight_ * path_a_eval.quality_score +
                path_b_weight_ * path_b_eval.quality_score;
            
            // Combine issues and suggestions
            decision.final_evaluation.issues = path_a_eval.issues;
            decision.final_evaluation.issues.insert(
                decision.final_evaluation.issues.end(),
                path_b_eval.issues.begin(),
                path_b_eval.issues.end()
            );
            
            decision.final_evaluation.suggestions = path_a_eval.suggestions;
            decision.final_evaluation.suggestions.insert(
                decision.final_evaluation.suggestions.end(),
                path_b_eval.suggestions.begin(),
                path_b_eval.suggestions.end()
            );
        } else if (decision.strategy == "path_a_dominant") {
            decision.final_evaluation = path_a_eval;
        } else {
            decision.final_evaluation = path_b_eval;
        }
        
        // Get improvements from both paths
        auto path_a_improvements = path_a_->suggest_improvements(features);
        
        QualityEvaluation quality_eval; // For Path-B
        quality_eval.overall_score = path_b_eval.quality_score;
        quality_eval.needs_improvement = (path_b_eval.quality_score < 80.0);
        auto path_b_improvements = path_b_->suggest_improvements(features, quality_eval, config_);
        
        // Merge improvements
        decision.improvements = path_a_improvements;
        decision.improvements.insert(
            decision.improvements.end(),
            path_b_improvements.begin(),
            path_b_improvements.end()
        );
        
        // Sort by priority and confidence
        std::sort(decision.improvements.begin(), decision.improvements.end(),
                 [](const auto& a, const auto& b) {
                     return (a.priority * a.confidence) > (b.priority * b.confidence);
                 });
        
        // Keep top 5
        if (decision.improvements.size() > 5) {
            decision.improvements.resize(5);
        }
        
        return decision;
    }
    
    // ==================== Store Knowledge ====================
    void store_knowledge_entry(const CodeFeatures& features,
                               const PathEvaluation& path_a_eval,
                               const PathEvaluation& path_b_eval,
                               const DualPathDecision& decision) {
        KnowledgeEntry entry;
        entry.features = features;
        entry.path_a_eval = path_a_eval;
        entry.path_b_eval = path_b_eval;
        entry.actual_quality = decision.final_evaluation.quality_score;
        entry.cycle_number = current_cycle_;
        entry.timestamp = std::chrono::system_clock::now();
        
        // Calculate errors
        entry.path_a_error = std::abs(path_a_eval.quality_score - entry.actual_quality);
        entry.path_b_error = std::abs(path_b_eval.quality_score - entry.actual_quality);
        
        knowledge_->add_entry(entry);
    }
    
    // ==================== Update Weights ====================
    void update_weights() {
        auto [wa, wb] = knowledge_->calculate_path_weights();
        
        // Smooth transition
        double alpha = 0.3; // Learning rate for weight updates
        path_a_weight_ = alpha * wa + (1 - alpha) * path_a_weight_;
        path_b_weight_ = alpha * wb + (1 - alpha) * path_b_weight_;
        
        std::cout << "\n[Dual-Cycle] Updated weights: A=" 
                  << path_a_weight_ << ", B=" << path_b_weight_ << "\n";
    }
    
    // ==================== Run Multiple Cycles ====================
    void run_cycles(int max_cycles = 10) {
        std::cout << "\n";
        std::cout << "██████╗ ██╗   ██╗ █████╗ ██╗      \n";
        std::cout << "██╔══██╗██║   ██║██╔══██╗██║      \n";
        std::cout << "██║  ██║██║   ██║███████║██║      \n";
        std::cout << "██║  ██║██║   ██║██╔══██║██║      \n";
        std::cout << "██████╔╝╚██████╔╝██║  ██║███████╗\n";
        std::cout << "╚═════╝  ╚═════╝ ╚═╝  ╚═╝╚══════╝\n";
        std::cout << "\n     Dual-Path DARV System\n";
        std::cout << "  Neural ⚡ + Symbolic 🧠 = 🚀\n\n";
        
        for (int i = 0; i < max_cycles; i++) {
            auto decision = run_single_cycle();
            
            // Check for convergence
            if (decision.final_evaluation.quality_score > 95.0) {
                std::cout << "\n✓ Excellent quality achieved! Stopping.\n";
                break;
            }
            
            // Short pause
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        
        print_final_summary();
        
        // Save everything
        save_state();
    }
    
    // ==================== Failed Decision ====================
    DualPathDecision create_failed_decision() {
        DualPathDecision decision;
        decision.final_evaluation.quality_score = 0.0;
        decision.final_evaluation.path_name = "FAILED";
        decision.paths_agree = false;
        decision.agreement_score = 0.0;
        decision.path_a_weight = path_a_weight_;
        decision.path_b_weight = path_b_weight_;
        decision.strategy = "none";
        return decision;
    }
    
    // ==================== Summary ====================
    void print_final_summary() {
        std::cout << "\n╔════════════════════════════════════════╗\n";
        std::cout << "║      Dual-Path Final Summary           ║\n";
        std::cout << "╚════════════════════════════════════════╝\n";
        
        std::cout << "\nCycles completed: " << decision_history_.size() << "\n";
        
        if (!decision_history_.empty()) {
            double avg_quality = 0.0;
            double avg_agreement = 0.0;
            
            for (const auto& dec : decision_history_) {
                avg_quality += dec.final_evaluation.quality_score;
                avg_agreement += dec.agreement_score;
            }
            
            avg_quality /= decision_history_.size();
            avg_agreement /= decision_history_.size();
            
            std::cout << "Average quality: " << avg_quality << "/100\n";
            std::cout << "Average agreement: " << (avg_agreement * 100) << "%\n";
            std::cout << "Final weights: A=" << path_a_weight_ 
                      << ", B=" << path_b_weight_ << "\n";
        }
        
        path_a_->print_stats();
        path_b_->print_stats();
        knowledge_->print_stats();
    }
    
    // ==================== Save State ====================
    void save_state() {
        std::cout << "\n[Dual-Cycle] Saving state...\n";
        
        // Ensure memory directory exists
        system("mkdir -p ./memory 2>/dev/null");
        
        path_a_->save(memory_path_ + "/path_a_model");
        knowledge_->save();
        
        std::cout << "[Dual-Cycle] State saved ✓\n";
    }
};

} // namespace dual_path
} // namespace darv

#endif // DARV_DUAL_CYCLE_HPP