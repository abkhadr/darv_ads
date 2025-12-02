#include "core/dual_path/dual_cycle.hpp"
#include <iostream>

using namespace darv;
using namespace darv::dual_path;

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                           ║\n";
    std::cout << "║        DARV Dual-Path System - Integration Test          ║\n";
    std::cout << "║        نظام DARV ثنائي المسار - اختبار التكامل           ║\n";
    std::cout << "║                                                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    
    // Setup project configuration
    ProjectConfig config;
    config.name = "test_complete_nn";
    config.root_path = "../";  // ⚡ FIX: من داخل build، الـ root هو ../
    config.build_commands = {
        "cmake ..",               // ⚡ FIX: نحن في build/ فعلاً
        "make test_complete_nn"
    };
    config.run_commands = {
        "./test_complete_nn"     // ⚡ FIX: في نفس المجلد
    };
    config.max_cycles = 100;
    config.convergence_threshold = 2.0;
    
    // Create dual-path cycle
    DualCycle dual_cycle(config, "./memory");
    
    // Run the system
    dual_cycle.run_cycles(config.max_cycles);
    
    std::cout << "\n✓ Dual-Path system test complete!\n\n";
    
    return 0;
}

/*
 * Expected Output:
 * 
 * ╔════════════════════════════════════════╗
 * ║   Dual-Path Cycle #1                   ║
 * ╚════════════════════════════════════════╝
 * 
 * ► Step 1/5: Build & Execute
 * [BUILD] Building project: test_complete_nn
 * [EXECUTOR] Result: Success ✓
 * 
 * ► Step 2/5: Feature Extraction
 * [Features] Extracted 13 features
 * 
 * ► Step 3/5: Path-A Evaluation (Neural)
 *   Quality: 65.3 (confidence: 0.75)
 * 
 * ► Step 4/5: Path-B Evaluation (Symbolic)
 *   Quality: 72.1 (confidence: 0.90)
 * 
 * ► Step 5/5: Dual-Path Decision
 * ╔════════════════════════════════════════╗
 * ║     Dual-Path Decision Summary         ║
 * ╚════════════════════════════════════════╝
 * Quality Score: 69.2/100
 * Paths Agree: Yes
 * Agreement: 93%
 * Path-A Weight: 0.5
 * Path-B Weight: 0.5
 * Strategy: weighted_average
 * Improvements: 4
 * 
 * ... (more cycles)
 * 
 * ╔════════════════════════════════════════╗
 * ║      Dual-Path Final Summary           ║
 * ╚════════════════════════════════════════╝
 * Cycles completed: 5
 * Average quality: 78.5/100
 * Average agreement: 88%
 * Final weights: A=0.6, B=0.4
 */