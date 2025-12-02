#include "core/darv_tensor.hpp"
#include "core/layers/nn_layers.hpp"
#include <iostream>
#include <iomanip>

using namespace darv::autograd;
using namespace darv::nn;

void debug_xor_training() {
    std::cout << "\n=== Debugging XOR Training ===" << std::endl;
    
    // Simple network
    Sequential model("debug_xor");
    model.add(std::make_shared<Linear>(2, 4, "hidden"));
    model.add(std::make_shared<ReLU>("relu"));
    model.add(std::make_shared<Linear>(4, 1, "output"));
    model.add(std::make_shared<Sigmoid>("sigmoid"));
    
    // Single XOR sample
    auto input = std::make_shared<Tensor>(
        std::vector<double>{1.0, 0.0}, Shape{2}, true
    );
    auto target = std::make_shared<Tensor>(
        std::vector<double>{1.0}, Shape{1}, false
    );
    
    std::cout << "\n--- Before Training ---" << std::endl;
    auto pred = model.forward(input);
    std::cout << "Prediction: " << pred->data()[0] << std::endl;
    
    // Print initial weights
    auto params = model.parameters();
    std::cout << "\nInitial weights (first layer):" << std::endl;
    for (size_t i = 0; i < 4; i++) {
        std::cout << "  w[" << i << "]: " << params[0]->data()[i] << std::endl;
    }
    
    // Single training step
    double lr = 0.5;
    for (int step = 0; step < 5; step++) {
        std::cout << "\n--- Training Step " << step << " ---" << std::endl;
        
        // Forward
        auto pred = model.forward(input);
        auto loss = MSELoss::compute(pred, target);
        
        std::cout << "Loss: " << loss->data()[0] << std::endl;
        std::cout << "Prediction: " << pred->data()[0] << std::endl;
        
        // Backward
        model.zero_grad();
        loss->backward();
        
        // Check gradients
        std::cout << "\nGradients:" << std::endl;
        bool has_gradients = false;
        for (size_t i = 0; i < params.size(); i++) {
            double grad_sum = 0.0;
            for (size_t j = 0; j < params[i]->size(); j++) {
                grad_sum += std::abs(params[i]->grad()[j]);
            }
            if (grad_sum > 1e-10) {
                has_gradients = true;
                std::cout << "  Param " << i << " (" << params[i]->name() 
                          << "): grad_sum = " << grad_sum << std::endl;
            }
        }
        
        if (!has_gradients) {
            std::cout << "  ❌ NO GRADIENTS COMPUTED!" << std::endl;
            std::cout << "\n=== PROBLEM IDENTIFIED ===" << std::endl;
            std::cout << "Gradients are not flowing back through the network." << std::endl;
            std::cout << "This means the backward pass is broken somewhere." << std::endl;
            return;
        } else {
            std::cout << "  ✓ Gradients computed successfully" << std::endl;
        }
        
        // Update
        for (auto& param : params) {
            for (size_t j = 0; j < param->size(); j++) {
                param->data()[j] -= lr * param->grad()[j];
            }
        }
        
        std::cout << "\nWeights after update (first layer, first 4):" << std::endl;
        for (size_t i = 0; i < 4; i++) {
            std::cout << "  w[" << i << "]: " << params[0]->data()[i] << std::endl;
        }
    }
    
    std::cout << "\n--- After Training ---" << std::endl;
    auto final_pred = model.forward(input);
    std::cout << "Final prediction: " << final_pred->data()[0] << std::endl;
    std::cout << "Target: " << target->data()[0] << std::endl;
}

int main() {
    srand(42);
    debug_xor_training();
    return 0;
}