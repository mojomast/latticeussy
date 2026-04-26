#include "greatest.h"
#include "lattice.h"
#include <string.h>
#include <stdio.h>

TEST add_cells_and_planes(void) {
    KnowledgeCrystal c; crystal_init(&c);
    int a = crystal_add_cell(&c, "Atom", 0.8, 0);
    int b = crystal_add_cell(&c, "Bond", 0.4, 1);
    ASSERT_EQ(0, a); ASSERT_EQ(1, b);
    ASSERT_EQ(0, crystal_add_plane(&c, a, b, "example"));
    ASSERT_EQ(2, c.cell_count); ASSERT_EQ(1, c.plane_count);
    PASS();
}

TEST rejects_invalid_planes(void) {
    KnowledgeCrystal c; crystal_init(&c);
    crystal_add_cell(&c, "Only", 0.5, 0);
    ASSERT_EQ(-1, crystal_add_plane(&c, 0, 0, "self"));
    ASSERT_EQ(-1, crystal_add_plane(&c, 0, 9, "missing"));
    ASSERT_EQ(0, c.plane_count);
    PASS();
}

TEST detects_defects_and_weak_cells(void) {
    KnowledgeCrystal c; crystal_init(&c);
    crystal_add_cell(&c, "Strong", 0.9, 0);
    crystal_add_cell(&c, "Gap", 0.2, 1);
    LatticeAnalysis a = crystal_analyze(&c);
    ASSERT_EQ(1, a.defects);
    ASSERT_EQ(1, a.weak_cells);
    ASSERT_EQ(1, a.next_review_index);
    ASSERT(strstr(a.recommendation, "defect") != NULL || strstr(a.recommendation, "Defect") != NULL || strstr(a.recommendation, "Anneal") != NULL);
    PASS();
}

TEST phase_transition_ready_when_connected_and_mastered(void) {
    KnowledgeCrystal c; crystal_init(&c);
    for (int i = 0; i < 5; ++i) { char n[16]; snprintf(n, sizeof(n), "C%d", i); crystal_add_cell(&c, n, 0.82, 0); }
    crystal_add_plane(&c,0,1,"cause"); crystal_add_plane(&c,1,2,"example"); crystal_add_plane(&c,2,3,"analogy"); crystal_add_plane(&c,3,4,"supports"); crystal_add_plane(&c,0,4,"synthesis");
    LatticeAnalysis a = crystal_analyze(&c);
    ASSERT_EQ(1, a.phase_transition_ready);
    ASSERT(a.crystallization_score > 65.0);
    PASS();
}

TEST sparse_crystal_recommends_links(void) {
    KnowledgeCrystal c; crystal_init(&c);
    crystal_add_cell(&c, "A", 0.8, 0); crystal_add_cell(&c, "B", 0.8, 0); crystal_add_cell(&c, "C", 0.8, 0);
    LatticeAnalysis a = crystal_analyze(&c);
    ASSERT(a.structural_density < 0.25);
    ASSERT(strstr(a.recommendation, "plane") != NULL || strstr(a.recommendation, "connect") != NULL);
    PASS();
}

TEST parses_text_fixture(void) {
    KnowledgeCrystal c;
    int n = parse_crystal_text("Alpha|0.9|0\nBeta|45|1\nGamma|0.6|0", "0->1|opposite\n1->2|example", &c);
    ASSERT_EQ(3, n);
    ASSERT_EQ(2, c.plane_count);
    ASSERT_EQ(1, c.cells[1].is_defect);
    ASSERT_IN_RANGE(0.45, c.cells[1].mastery, 0.001);
    PASS();
}

TEST json_contains_domain_terms(void) {
    KnowledgeCrystal c; crystal_sample(&c);
    LatticeAnalysis a = crystal_analyze(&c);
    char json[4096]; analysis_to_json(&c, &a, json, sizeof(json));
    ASSERT(strstr(json, "crystallization_score") != NULL);
    ASSERT(strstr(json, "bravais_frame") != NULL);
    ASSERT(strstr(json, "next_review") != NULL);
    PASS();
}

TEST sample_has_defect_and_relations(void) {
    KnowledgeCrystal c; crystal_sample(&c);
    LatticeAnalysis a = crystal_analyze(&c);
    ASSERT(c.cell_count >= 5);
    ASSERT(c.plane_count >= 5);
    ASSERT_EQ(1, a.defects);
    PASS();
}

SUITE(lattice_suite) {
    RUN_TEST(add_cells_and_planes);
    RUN_TEST(rejects_invalid_planes);
    RUN_TEST(detects_defects_and_weak_cells);
    RUN_TEST(phase_transition_ready_when_connected_and_mastered);
    RUN_TEST(sparse_crystal_recommends_links);
    RUN_TEST(parses_text_fixture);
    RUN_TEST(json_contains_domain_terms);
    RUN_TEST(sample_has_defect_and_relations);
}

GREATEST_MAIN_DEFS();
int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(lattice_suite);
    GREATEST_MAIN_END();
}
