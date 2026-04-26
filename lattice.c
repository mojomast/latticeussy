#define _GNU_SOURCE
#include "lattice.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

static void copy_field(char *dst, size_t n, const char *src) {
    if (!dst || n == 0) return;
    if (!src) src = "";
    snprintf(dst, n, "%s", src);
}

static double clamp(double v, double lo, double hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void crystal_init(KnowledgeCrystal *crystal) {
    if (!crystal) return;
    memset(crystal, 0, sizeof(*crystal));
}

int crystal_add_cell(KnowledgeCrystal *crystal, const char *name, double mastery, int is_defect) {
    if (!crystal || crystal->cell_count >= LATTICE_MAX_CELLS || !name || !*name) return -1;
    int i = crystal->cell_count++;
    copy_field(crystal->cells[i].name, sizeof(crystal->cells[i].name), name);
    crystal->cells[i].mastery = clamp(mastery, 0.0, 1.0);
    crystal->cells[i].is_defect = is_defect ? 1 : 0;
    return i;
}

int crystal_add_plane(KnowledgeCrystal *crystal, int from, int to, const char *type) {
    if (!crystal || crystal->plane_count >= LATTICE_MAX_LINKS) return -1;
    if (from < 0 || to < 0 || from >= crystal->cell_count || to >= crystal->cell_count || from == to) return -1;
    int i = crystal->plane_count++;
    crystal->planes[i].from = from;
    crystal->planes[i].to = to;
    copy_field(crystal->planes[i].type, sizeof(crystal->planes[i].type), type && *type ? type : "related");
    return i;
}

static int degree_of(const KnowledgeCrystal *c, int idx) {
    int d = 0;
    for (int i = 0; i < c->plane_count; ++i) {
        if (c->planes[i].from == idx || c->planes[i].to == idx) d++;
    }
    return d;
}

LatticeAnalysis crystal_analyze(const KnowledgeCrystal *crystal) {
    LatticeAnalysis a;
    memset(&a, 0, sizeof(a));
    if (!crystal) return a;
    a.unit_cells = crystal->cell_count;
    a.lattice_planes = crystal->plane_count;
    double mastery_sum = 0.0;
    double best_priority = -999.0;
    a.next_review_index = -1;
    for (int i = 0; i < crystal->cell_count; ++i) {
        const UnitCell *cell = &crystal->cells[i];
        mastery_sum += cell->mastery;
        if (cell->is_defect) a.defects++;
        if (cell->mastery < 0.55) a.weak_cells++;
        int deg = degree_of(crystal, i);
        double priority = (1.0 - cell->mastery) * 2.0 + (deg == 0 ? 1.0 : 0.0) + (cell->is_defect ? 2.5 : 0.0);
        if (priority > best_priority) {
            best_priority = priority;
            a.next_review_index = i;
        }
    }
    a.average_mastery = crystal->cell_count ? mastery_sum / crystal->cell_count : 0.0;
    double possible = crystal->cell_count > 1 ? (double)crystal->cell_count * (crystal->cell_count - 1) / 2.0 : 1.0;
    a.structural_density = clamp((double)crystal->plane_count / possible, 0.0, 1.0);
    double defect_penalty = crystal->cell_count ? (double)a.defects / crystal->cell_count : 0.0;
    a.crystallization_score = clamp(100.0 * (0.55 * a.average_mastery + 0.35 * a.structural_density + 0.10 * (1.0 - defect_penalty)), 0.0, 100.0);
    a.phase_transition_ready = (a.unit_cells >= 4 && a.lattice_planes >= a.unit_cells && a.average_mastery >= 0.68 && a.defects <= 1);
    if (a.structural_density < 0.18) copy_field(a.bravais_frame, sizeof(a.bravais_frame), "powder: isolated facts");
    else if (a.structural_density < 0.42) copy_field(a.bravais_frame, sizeof(a.bravais_frame), "orthorhombic: organized strands");
    else copy_field(a.bravais_frame, sizeof(a.bravais_frame), "cubic: mutually reinforcing framework");
    if (a.unit_cells == 0) copy_field(a.recommendation, sizeof(a.recommendation), "Add three unit cells: a core idea, an example, and a counterexample.");
    else if (a.defects > 0) copy_field(a.recommendation, sizeof(a.recommendation), "Anneal red defect cells first: correct misconceptions before adding more facts.");
    else if (a.structural_density < 0.25) copy_field(a.recommendation, sizeof(a.recommendation), "Grow lattice planes: connect concepts with cause, example, analogy, or contrast links.");
    else if (a.phase_transition_ready) copy_field(a.recommendation, sizeof(a.recommendation), "Phase transition ready: quiz across planes and write a synthesis paragraph.");
    else copy_field(a.recommendation, sizeof(a.recommendation), "Use X-ray review on the weakest connected unit cell, then add one bridge concept.");
    return a;
}

void crystal_sample(KnowledgeCrystal *c) {
    crystal_init(c);
    int a = crystal_add_cell(c, "Spaced repetition interval", 0.82, 0);
    int b = crystal_add_cell(c, "Retrieval practice", 0.74, 0);
    int d = crystal_add_cell(c, "Interleaving", 0.51, 0);
    int e = crystal_add_cell(c, "Misconception: rereading equals learning", 0.28, 1);
    int f = crystal_add_cell(c, "Transfer to new problems", 0.67, 0);
    crystal_add_plane(c, a, b, "cause");
    crystal_add_plane(c, b, d, "reinforces");
    crystal_add_plane(c, d, f, "enables");
    crystal_add_plane(c, e, b, "opposite");
    crystal_add_plane(c, a, f, "supports");
}

static void json_escape(char *out, size_t out_size, const char *s) {
    size_t j = 0;
    for (size_t i = 0; s && s[i] && j + 2 < out_size; ++i) {
        unsigned char ch = (unsigned char)s[i];
        if (ch == '"' || ch == '\\') { out[j++]='\\'; out[j++]=(char)ch; }
        else if (ch == '\n' || ch == '\r') out[j++]=' ';
        else if (ch >= 32) out[j++]=(char)ch;
    }
    out[j] = 0;
}

void analysis_to_json(const KnowledgeCrystal *c, const LatticeAnalysis *a, char *out, size_t out_size) {
    char rec[512], frame[128];
    json_escape(rec, sizeof(rec), a->recommendation);
    json_escape(frame, sizeof(frame), a->bravais_frame);
    int n = snprintf(out, out_size,
        "{\"unit_cells\":%d,\"lattice_planes\":%d,\"defects\":%d,\"weak_cells\":%d,\"average_mastery\":%.3f,\"structural_density\":%.3f,\"crystallization_score\":%.1f,\"phase_transition_ready\":%s,\"bravais_frame\":\"%s\",\"recommendation\":\"%s\",\"next_review\":",
        a->unit_cells, a->lattice_planes, a->defects, a->weak_cells, a->average_mastery,
        a->structural_density, a->crystallization_score, a->phase_transition_ready ? "true" : "false", frame, rec);
    if (n < 0 || (size_t)n >= out_size) return;
    size_t pos = (size_t)n;
    if (a->next_review_index >= 0 && c && a->next_review_index < c->cell_count) {
        char cell[128]; json_escape(cell, sizeof(cell), c->cells[a->next_review_index].name);
        n = snprintf(out + pos, out_size - pos, "\"%s\"", cell);
    } else n = snprintf(out + pos, out_size - pos, "null");
    if (n < 0) return;
    pos += (size_t)n;
    snprintf(out + pos, out_size - pos, "}");
}

void analysis_to_text(const KnowledgeCrystal *c, const LatticeAnalysis *a, char *out, size_t out_size) {
    const char *next = (a->next_review_index >= 0 && c && a->next_review_index < c->cell_count) ? c->cells[a->next_review_index].name : "none yet";
    snprintf(out, out_size,
        "Lattice analysis\nUnit cells: %d\nLattice planes: %d\nDefects: %d\nAverage mastery: %.0f%%\nStructural density: %.0f%%\nCrystallization score: %.1f/100\nFrame: %s\nNext X-ray review: %s\nRecommendation: %s\n",
        a->unit_cells, a->lattice_planes, a->defects, a->average_mastery * 100.0,
        a->structural_density * 100.0, a->crystallization_score, a->bravais_frame, next, a->recommendation);
}

static void trim(char *s) {
    if (!s) return;
    char *start = s;
    while (isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n-1])) s[--n] = 0;
}

int parse_crystal_text(const char *concepts, const char *relations, KnowledgeCrystal *crystal) {
    if (!crystal) return -1;
    crystal_init(crystal);
    if (concepts && *concepts) {
        char buf[8192]; copy_field(buf, sizeof(buf), concepts);
        char *save = NULL;
        for (char *line = strtok_r(buf, "\n;", &save); line; line = strtok_r(NULL, "\n;", &save)) {
            trim(line); if (!*line) continue;
            char *m = strchr(line, '|');
            char *d = m ? strchr(m + 1, '|') : NULL;
            double mastery = 0.5; int defect = 0;
            if (m) { *m = 0; mastery = atof(m + 1); if (mastery > 1.0) mastery /= 100.0; }
            if (d) { *d = 0; defect = atoi(d + 1); }
            trim(line);
            crystal_add_cell(crystal, line, mastery, defect);
        }
    }
    if (relations && *relations) {
        char buf[8192]; copy_field(buf, sizeof(buf), relations);
        char *save = NULL;
        for (char *line = strtok_r(buf, "\n;", &save); line; line = strtok_r(NULL, "\n;", &save)) {
            trim(line); if (!*line) continue;
            int from=-1,to=-1; char type[64]="related";
            if (sscanf(line, "%d->%d|%63s", &from, &to, type) >= 2) crystal_add_plane(crystal, from, to, type);
        }
    }
    return crystal->cell_count;
}
