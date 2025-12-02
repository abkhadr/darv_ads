#ifndef DARV_KNOWLEDGE_BASE_HPP
#define DARV_KNOWLEDGE_BASE_HPP

#include "path_types.hpp"
#include <vector>
#include <fstream>
#include <algorithm>
#include <numeric>

namespace darv {
namespace dual_path {

// ==================== Knowledge Base ====================
// قاعدة معرفة تجمع خبرات Path-A و Path-B
class KnowledgeBase {
private:
    std::vector<KnowledgeEntry> entries_;
    size_t max_capacity_ = 10000;
    std::string storage_path_;
    
    // Statistics
    double path_a_avg_error_ = 0.0;
    double path_b_avg_error_ = 0.0;
    size_t num_entries_analyzed_ = 0;

public:
    KnowledgeBase(const std::string& storage_path = "./memory/knowledge_base")
        : storage_path_(storage_path) {}
    
    // ==================== Add Entry ====================
    void add_entry(const KnowledgeEntry& entry) {
        entries_.push_back(entry);
        
        // Update statistics
        path_a_avg_error_ = (path_a_avg_error_ * num_entries_analyzed_ + entry.path_a_error) /
                           (num_entries_analyzed_ + 1);
        path_b_avg_error_ = (path_b_avg_error_ * num_entries_analyzed_ + entry.path_b_error) /
                           (num_entries_analyzed_ + 1);
        num_entries_analyzed_++;
        
        // Limit capacity (keep most recent)
        if (entries_.size() > max_capacity_) {
            entries_.erase(entries_.begin());
        }
        
        std::cout << "[Knowledge Base] Added entry (total: " << entries_.size() << ")" << std::endl;
    }
    
    // ==================== Query Similar ====================
    std::vector<KnowledgeEntry> query_similar(const CodeFeatures& features, size_t top_k = 5) {
        if (entries_.empty()) {
            return {};
        }
        
        // Calculate similarity scores
        std::vector<std::pair<double, size_t>> scores;
        for (size_t i = 0; i < entries_.size(); i++) {
            double similarity = calculate_similarity(features, entries_[i].features);
            scores.push_back({similarity, i});
        }
        
        // Sort by similarity
        std::sort(scores.begin(), scores.end(),
                 [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // Return top-k
        std::vector<KnowledgeEntry> results;
        for (size_t i = 0; i < std::min(top_k, scores.size()); i++) {
            results.push_back(entries_[scores[i].second]);
        }
        
        return results;
    }
    
    // ==================== Calculate Path Weights ====================
    std::pair<double, double> calculate_path_weights() const {
        if (num_entries_analyzed_ == 0) {
            return {0.5, 0.5}; // Equal weights initially
        }
        
        // Lower error = higher weight
        double total_error = path_a_avg_error_ + path_b_avg_error_;
        
        if (total_error < 1e-6) {
            return {0.5, 0.5};
        }
        
        // Inverse error weighting
        double path_a_weight = path_b_avg_error_ / total_error;
        double path_b_weight = path_a_avg_error_ / total_error;
        
        // Normalize
        double sum = path_a_weight + path_b_weight;
        path_a_weight /= sum;
        path_b_weight /= sum;
        
        // Apply smoothing (don't let one path dominate too much)
        path_a_weight = std::max(0.2, std::min(0.8, path_a_weight));
        path_b_weight = 1.0 - path_a_weight;
        
        return {path_a_weight, path_b_weight};
    }
    
    // ==================== Analyze Agreement ====================
    double analyze_agreement() const {
        if (entries_.size() < 2) {
            return 0.5;
        }
        
        int agreements = 0;
        int total = 0;
        
        for (const auto& entry : entries_) {
            double diff = std::abs(entry.path_a_eval.quality_score - 
                                  entry.path_b_eval.quality_score);
            
            // Consider agreement if within 10 points
            if (diff < 10.0) {
                agreements++;
            }
            total++;
        }
        
        return static_cast<double>(agreements) / total;
    }
    
    // ==================== Get Best Performing Path ====================
    std::string get_best_path() const {
        if (num_entries_analyzed_ == 0) {
            return "Unknown";
        }
        
        return (path_a_avg_error_ < path_b_avg_error_) ? "Path-A" : "Path-B";
    }
    
    // ==================== Save/Load ====================
    bool save() const {
        std::ofstream file(storage_path_ + ".dat", std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        // Simple text format for now
        file << entries_.size() << "\n";
        file << path_a_avg_error_ << "\n";
        file << path_b_avg_error_ << "\n";
        file << num_entries_analyzed_ << "\n";
        
        // Save basic statistics only (full entries would be too large)
        for (const auto& entry : entries_) {
            file << entry.cycle_number << " ";
            file << entry.actual_quality << " ";
            file << entry.path_a_error << " ";
            file << entry.path_b_error << "\n";
        }
        
        file.close();
        std::cout << "[Knowledge Base] Saved to " << storage_path_ << ".dat" << std::endl;
        return true;
    }
    
    bool load() {
        std::ifstream file(storage_path_ + ".dat", std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        size_t size;
        file >> size;
        file >> path_a_avg_error_;
        file >> path_b_avg_error_;
        file >> num_entries_analyzed_;
        
        entries_.clear();
        
        for (size_t i = 0; i < size; i++) {
            KnowledgeEntry entry;
            file >> entry.cycle_number;
            file >> entry.actual_quality;
            file >> entry.path_a_error;
            file >> entry.path_b_error;
            
            entries_.push_back(entry);
        }
        
        file.close();
        std::cout << "[Knowledge Base] Loaded " << entries_.size() << " entries" << std::endl;
        return true;
    }
    
    // ==================== Statistics ====================
    void print_stats() const {
        std::cout << "\n╔════════════════════════════════════════╗\n";
        std::cout << "║       Knowledge Base Statistics        ║\n";
        std::cout << "╚════════════════════════════════════════╝\n";
        std::cout << "Total entries: " << entries_.size() << "\n";
        std::cout << "Path-A avg error: " << path_a_avg_error_ << "\n";
        std::cout << "Path-B avg error: " << path_b_avg_error_ << "\n";
        std::cout << "Best path: " << get_best_path() << "\n";
        std::cout << "Agreement rate: " << (analyze_agreement() * 100) << "%\n";
        
        auto [wa, wb] = calculate_path_weights();
        std::cout << "Recommended weights: A=" << wa << ", B=" << wb << "\n";
    }
    
    size_t size() const { return entries_.size(); }
    const std::vector<KnowledgeEntry>& entries() const { return entries_; }

private:
    // ==================== Similarity Calculation ====================
    double calculate_similarity(const CodeFeatures& f1, const CodeFeatures& f2) const {
        // Euclidean distance in normalized feature space
        double dist = 0.0;
        
        dist += std::pow((f1.lines_of_code - f2.lines_of_code) / 1000.0, 2);
        dist += std::pow((f1.cyclomatic_complexity - f2.cyclomatic_complexity) / 50.0, 2);
        dist += std::pow((f1.execution_time_ms - f2.execution_time_ms) / 1000.0, 2);
        dist += std::pow((f1.compile_errors - f2.compile_errors), 2);
        dist += std::pow((f1.warnings - f2.warnings) / 10.0, 2);
        
        // Convert distance to similarity [0,1]
        return 1.0 / (1.0 + std::sqrt(dist));
    }
};

} // namespace dual_path
} // namespace darv

#endif // DARV_KNOWLEDGE_BASE_HPP