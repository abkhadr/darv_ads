#include "core/dual_path/dual_cycle.hpp"
#include <iostream>
#include <unistd.h>

using namespace darv;
using namespace darv::dual_path;

// ==================== Mock Test (No Real Build) ====================
// للاختبار بدون بناء حقيقي
void test_dual_path_mock() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                           ║\n";
    std::cout << "║        DARV Dual-Path System - Mock Test                 ║\n";
    std::cout << "║        (Testing Core Logic Without Real Build)           ║\n";
    std::cout << "║                                                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    
    // Create Path-A and Path-B
    PathA path_a;
    PathB path_b;
    KnowledgeBase knowledge("./memory/knowledge_base");
    
    std::cout << "✓ Components initialized\n\n";
    
    // Simulate 5 cycles with mock data
    for (int cycle = 1; cycle <= 5; cycle++) {
        std::cout << "╔════════════════════════════════════════╗\n";
        std::cout << "║   Mock Cycle #" << cycle << "                          ║\n";
        std::cout << "╚════════════════════════════════════════╝\n\n";
        
        // Mock features
        CodeFeatures features;
        features.lines_of_code = 500 + cycle * 50;
        features.num_functions = 20 + cycle * 2;
        features.cyclomatic_complexity = 15 + cycle;
        features.execution_time_ms = 100.0 + cycle * 10;
        features.compile_errors = (cycle < 3) ? 1 : 0;
        features.runtime_errors = 0;
        features.warnings = 5 - cycle;
        features.code_coverage = 0.6 + cycle * 0.05;
        features.test_passed = 10 + cycle * 2;
        features.test_failed = 5 - cycle;
        features.exit_code = (cycle < 3) ? 1 : 0;
        features.memory_usage_kb = 1000 + cycle * 100;
        
        // Path-A Evaluation
        std::cout << "► Path-A Evaluation (Neural)\n";
        PathEvaluation path_a_eval = path_a.evaluate(features);
        std::cout << "  Quality: " << path_a_eval.quality_score 
                  << " (confidence: " << path_a_eval.confidence << ")\n\n";
        
        // Path-B Evaluation (Mock ExecutionResult)
        std::cout << "► Path-B Evaluation (Symbolic)\n";
        ExecutionResult mock_exec;
        mock_exec.success = (cycle >= 3);
        mock_exec.execution_time_ms = features.execution_time_ms;
        mock_exec.exit_code = features.exit_code;
        mock_exec.stderr_output = (cycle < 3) ? "error: something failed" : "";
        
        PathEvaluation path_b_eval = path_b.evaluate(features, mock_exec);
        std::cout << "  Quality: " << path_b_eval.quality_score 
                  << " (confidence: " << path_b_eval.confidence << ")\n\n";
        
        // Calculate agreement
        double agreement = 1.0 - std::abs(path_a_eval.quality_score - path_b_eval.quality_score) / 100.0;
        bool agree = (std::abs(path_a_eval.quality_score - path_b_eval.quality_score) < 15.0);
        
        std::cout << "► Decision\n";
        std::cout << "  Paths agree: " << (agree ? "Yes" : "No") << "\n";
        std::cout << "  Agreement: " << (agreement * 100) << "%\n";
        
        // Mock actual quality (simulating gradual improvement)
        double actual_quality = 50.0 + cycle * 8.0;
        
        // Learn from feedback
        path_a.learn_from_feedback(features, actual_quality, true);
        path_b.learn_from_feedback(features, actual_quality, path_b_eval.quality_score);
        
        // Store knowledge
        KnowledgeEntry entry;
        entry.features = features;
        entry.path_a_eval = path_a_eval;
        entry.path_b_eval = path_b_eval;
        entry.actual_quality = actual_quality;
        entry.cycle_number = cycle;
        entry.path_a_error = std::abs(path_a_eval.quality_score - actual_quality);
        entry.path_b_error = std::abs(path_b_eval.quality_score - actual_quality);
        entry.timestamp = std::chrono::system_clock::now();
        
        knowledge.add_entry(entry);
        
        std::cout << "  Actual quality: " << actual_quality << "\n";
        std::cout << "  Path-A error: " << entry.path_a_error << "\n";
        std::cout << "  Path-B error: " << entry.path_b_error << "\n\n";
    }
    
    // Print final statistics
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║         Final Statistics               ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    path_a.print_stats();
    path_b.print_stats();
    knowledge.print_stats();
    
    // Save state
    system("mkdir -p ./memory 2>/dev/null");
    path_a.save("./memory/path_a_model");
    knowledge.save();
    
    std::cout << "\n✓ Mock test complete!\n";
    std::cout << "✓ Models and knowledge saved to ./memory/\n\n";
}

// ==================== Real Test (With Actual Build) ====================
void test_dual_path_real() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                           ║\n";
    std::cout << "║        DARV Dual-Path System - Real Test                 ║\n";
    std::cout << "║        (With Actual Project Build & Execution)           ║\n";
    std::cout << "║                                                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    
    // Get current working directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        std::cerr << "Error getting current directory\n";
        return;
    }
    
    std::string current_dir(cwd);
    std::cout << "Current directory: " << current_dir << "\n\n";
    
    // Setup project configuration
    ProjectConfig config;
    config.name = "test_complete_nn";
    config.root_path = "../";
    
    // Simple commands that work from build/
    config.build_commands = {
        "make test_complete_nn 2>&1"  // Rebuild if needed
    };
    config.run_commands = {
        "./test_complete_nn 2>&1 | head -30"  // Show first 30 lines
    };
    config.max_cycles = 3;  // Just 3 cycles for real test
    config.convergence_threshold = 2.0;
    
    // Create dual-path cycle
    DualCycle dual_cycle(config, "./memory");
    
    // Run the system
    dual_cycle.run_cycles(config.max_cycles);
    
    std::cout << "\n✓ Real test complete!\n\n";
}

int main(int argc, char* argv[]) {
    // Check mode
    std::string mode = "mock";
    if (argc > 1) {
        mode = argv[1];
    }
    
    if (mode == "real") {
        test_dual_path_real();
    } else {
        test_dual_path_mock();
    }
    
    return 0;
}

/*
 * Usage:
 *   ./test_dual_path          -> Mock test (no real build)
 *   ./test_dual_path real     -> Real test (actual build)
 */