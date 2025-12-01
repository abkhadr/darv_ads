#ifndef DARV_EXECUTOR_HPP
#define DARV_EXECUTOR_HPP

#include "darv_types.hpp"
#include <memory>
#include <cstdio>
#include <array>
#include <sstream>
#include <iostream>

namespace darv {

class Executor {
public:
    Executor() = default;
    
    // تنفيذ أمر في الـ shell
    ExecutionResult execute_command(const std::string& command, int timeout_seconds = 30) {
        ExecutionResult result;
        auto start = std::chrono::high_resolution_clock::now();
        result.timestamp = std::chrono::system_clock::now();
        
        std::cout << "[EXECUTOR] تنفيذ: " << command << std::endl;
        
        // تنفيذ الأمر وقراءة النتيجة
        std::array<char, 128> buffer;
        std::string output;
        
        FILE* pipe = popen((command + " 2>&1").c_str(), "r");
        if (!pipe) {
            result.success = false;
            result.stderr_output = "فشل في فتح الـ pipe";
            return result;
        }
        
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            output += buffer.data();
        }
        
        int exit_code = pclose(pipe);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        
        result.exit_code = WEXITSTATUS(exit_code);
        result.success = (result.exit_code == 0);
        result.stdout_output = output;
        result.execution_time_ms = elapsed.count();
        
        std::cout << "[EXECUTOR] النتيجة: " 
                  << (result.success ? "نجح ✓" : "فشل ✗")
                  << " (الوقت: " << result.execution_time_ms << " ms)" << std::endl;
        
        return result;
    }
    
    // بناء المشروع
    ExecutionResult build_project(const ProjectConfig& config) {
        std::cout << "\n[BUILD] بناء المشروع: " << config.name << std::endl;
        
        ExecutionResult result;
        result.success = true;
        result.timestamp = std::chrono::system_clock::now();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (const auto& cmd : config.build_commands) {
            ExecutionResult cmd_result = execute_command(cmd);
            result.stdout_output += cmd_result.stdout_output + "\n";
            result.stderr_output += cmd_result.stderr_output + "\n";
            
            if (!cmd_result.success) {
                result.success = false;
                result.exit_code = cmd_result.exit_code;
                break;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        result.execution_time_ms = elapsed.count();
        
        return result;
    }
    
    // تشغيل المشروع
    ExecutionResult run_project(const ProjectConfig& config) {
        std::cout << "\n[RUN] تشغيل المشروع: " << config.name << std::endl;
        
        ExecutionResult result;
        result.success = true;
        result.timestamp = std::chrono::system_clock::now();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (const auto& cmd : config.run_commands) {
            ExecutionResult cmd_result = execute_command(cmd);
            result.stdout_output += cmd_result.stdout_output + "\n";
            result.stderr_output += cmd_result.stderr_output + "\n";
            result.exit_code = cmd_result.exit_code;
            result.success = cmd_result.success;
            
            if (!cmd_result.success) {
                break;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        result.execution_time_ms = elapsed.count();
        
        return result;
    }
};

} // namespace darv

#endif // DARV_EXECUTOR_HPP