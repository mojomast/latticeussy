#ifndef LATTICE_H
#define LATTICE_H

#include <stddef.h>

#define LATTICE_MAX_CELLS 64
#define LATTICE_MAX_LINKS 128
#define LATTICE_NAME_LEN 80
#define LATTICE_TYPE_LEN 32

typedef struct {
    char name[LATTICE_NAME_LEN];
    double mastery;          /* 0.0 unknown, 1.0 fluent */
    int is_defect;           /* misconception/gap marker */
} UnitCell;

typedef struct {
    int from;
    int to;
    char type[LATTICE_TYPE_LEN]; /* cause/example/opposite/analogy/etc */
} LatticePlane;

typedef struct {
    UnitCell cells[LATTICE_MAX_CELLS];
    int cell_count;
    LatticePlane planes[LATTICE_MAX_LINKS];
    int plane_count;
} KnowledgeCrystal;

typedef struct {
    int unit_cells;
    int lattice_planes;
    int defects;
    int weak_cells;
    double average_mastery;
    double structural_density;
    double crystallization_score;
    int phase_transition_ready;
    int next_review_index;
    char bravais_frame[64];
    char recommendation[256];
} LatticeAnalysis;

void crystal_init(KnowledgeCrystal *crystal);
int crystal_add_cell(KnowledgeCrystal *crystal, const char *name, double mastery, int is_defect);
int crystal_add_plane(KnowledgeCrystal *crystal, int from, int to, const char *type);
LatticeAnalysis crystal_analyze(const KnowledgeCrystal *crystal);
void crystal_sample(KnowledgeCrystal *crystal);
void analysis_to_json(const KnowledgeCrystal *crystal, const LatticeAnalysis *analysis, char *out, size_t out_size);
void analysis_to_text(const KnowledgeCrystal *crystal, const LatticeAnalysis *analysis, char *out, size_t out_size);
int parse_crystal_text(const char *concepts, const char *relations, KnowledgeCrystal *crystal);

#endif
