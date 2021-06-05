#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <cglm/cglm.h>

struct pgrid_point {
	vec3 pos;
	versor rot;
	char *path;
	size_t path_sz;

	size_t rank;
	size_t width, height;
	unsigned char *data;

	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

struct pgrid_grid {
	struct pgrid_point *points;
	size_t points_ln;
	size_t rank_zero_idx;
	size_t raw_points;
	vec3 rank_pos;

	pthread_mutex_t mutex;
	pthread_cond_t cond;

	struct {
		uint64_t decoded, evicted, waits;
		double wait_time;
	} metrics;
};

struct pgrid_node_sphere {
	GLuint program, texture, vao, vbo;
	size_t elements;
	ssize_t point_idx;
};

struct pgrid_node_minimap {
	GLuint program, vao, vbo;
	size_t points;
};

struct pgrid_scene {
	struct pgrid_node_sphere sphere;
	struct pgrid_node_minimap minimap;
};

struct pgrid {
	struct pgrid_scene scene;
	struct pgrid_grid *grid;
	size_t width, height;
	float fov;
	float interp_scale;
	bool minimap;

	struct {
		uint64_t frames;
		double frame_time, max_frame_time;
	} metrics;
};

void pgrid_grid_init(struct pgrid_grid *grid, size_t raw_points);

void pgrid_grid_load(struct pgrid_grid *grid, const char *path, size_t path_sz);

void pgrid_grid_single(struct pgrid_grid *grid, const char *path,
	size_t path_sz);

void pgrid_grid_finish(struct pgrid_grid *grid);

void pgrid_init(struct pgrid *pgrid, struct pgrid_grid *grid, size_t width,
	size_t height, float fov);

void pgrid_render(struct pgrid* pgrid, vec3 pos, versor rot);

void pgrid_finish(struct pgrid *pgrid);

void pgrid_threads_init(struct pgrid_grid *grid, pthread_t *threads,
	size_t threads_ln);

void pgrid_threads_finish(struct pgrid_grid *grid, pthread_t *threads,
	size_t threads_ln);

void pgrid_metrics_print(FILE *file, struct pgrid *pgrid,
	struct pgrid_grid *grid);

void pgrid_quat_euler(float pitch, float yaw, float roll, versor dest);
