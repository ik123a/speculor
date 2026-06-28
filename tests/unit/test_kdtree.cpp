#include <gtest/gtest.h>
#include "speculor/detectors/kdtree_detector.hpp"
#include <vector>

TEST(KDTreeTest, BasicOperations) {
    speculor::KDTree tree;
    
    std::vector<double> p1 = {1.0, 2.0};
    std::vector<double> p2 = {4.0, 6.0};
    std::vector<double> query = {2.0, 3.0};
    
    tree.insert(p1);
    tree.insert(p2);
    
    // distance from query (2,3) to p1 (1,2) is sqrt((2-1)^2 + (3-2)^2) = sqrt(2) ≈ 1.414
    // distance from query (2,3) to p2 (4,6) is sqrt((2-4)^2 + (3-6)^2) = sqrt(4 + 9) = sqrt(13) ≈ 3.605
    double dist = tree.nearest_distance(query, 1);
    EXPECT_NEAR(dist, 1.414213, 1e-4);
}

TEST(KDTreeTest, ClearAndBoundary) {
    speculor::KDTree tree;
    std::vector<double> p = {10.0, 20.0};
    
    tree.insert(p);
    EXPECT_DOUBLE_EQ(tree.nearest_distance(p, 1), 0.0);
    
    tree.clear();
    // nearest distance on empty tree should handle gracefully (typically returns huge number or 0 depending on implementation)
    // we just check clear doesn't crash
}
