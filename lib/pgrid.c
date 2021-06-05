#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <turbojpeg.h>
#include <stdlib.h>
#include <string.h>
#include <cglm/cglm.h>

#include "glad/gl.h"
#include "pgrid/pgrid.h"
#include "pgrid/log.h"

static double
timespec_diff(struct timespec start, struct timespec end)
{
	struct timespec diff = {
		.tv_sec = end.tv_sec - start.tv_sec,
		.tv_nsec = end.tv_nsec - start.tv_nsec,
	};

	if (diff.tv_nsec < 0) {
		diff.tv_nsec += 1000000000;
		diff.tv_sec -= 1;
	}

	return diff.tv_sec + diff.tv_nsec / 1000000000.0;
}

static GLuint
program_create(const GLchar *vertex_src, const GLchar *fragment_src)
{
	GLuint vertex_shader, fragment_shader, program;
	GLint status, log_sz;
	GLsizei log_ln;

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	assert(vertex_shader);

	glShaderSource(vertex_shader, 1, &vertex_src, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_sz);
		GLchar vlog[log_sz];
		glGetShaderInfoLog(vertex_shader, log_sz, &log_ln, vlog);
		pgrid_log(PGRID_CRITICAL, "Vertex shader compilation failed: "
			"\n%s",	vlog);
	}
	assert(status);

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	assert(fragment_shader);

	glShaderSource(fragment_shader, 1, &fragment_src, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_sz);
		GLchar flog[log_sz];
		glGetShaderInfoLog(fragment_shader, log_sz, &log_ln, flog);
		pgrid_log(PGRID_CRITICAL, "Fragment shader compilation failed: "
			"\n%s", flog);
	}
	assert(status);

	program = glCreateProgram();
	assert(program);
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		glGetShaderiv(program, GL_INFO_LOG_LENGTH, &log_sz);
		GLchar llog[log_sz];
		glGetShaderInfoLog(vertex_shader, log_sz, &log_ln, llog);
		pgrid_log(PGRID_CRITICAL, "Shader program linking failed: "
			"\n%s", llog);
	}
	assert(status);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program;
}

static void
node_sphere_init(struct pgrid_node_sphere *sphere)
{
	/* Config */

	static const size_t ures = 300;
	static const size_t vres = 150;


	/* Program */

	static const GLchar *vs_src = "#version 460 core\n"
		"layout (location = 0) in vec3 pos;\n"
		"layout (location = 1) in vec2 uv;\n"
		"out vec2 tex;\n"
		"uniform mat4 mvp;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = mvp * vec4(pos, 1.0f);\n"
		"	tex = uv.xy;\n"
		"}\n";

	static const GLchar *fs_src = "#version 460 core\n"
		"in vec2 tex;\n"
		"out vec4 color;\n"
		"uniform sampler2D sampler;\n"
		"void main()\n"
		"{\n"
		"       color = texture(sampler, tex);\n"
		"}\n";

	sphere->program = program_create(vs_src, fs_src);
	glUseProgram(sphere->program);
	glUniform1i(glGetUniformLocation(sphere->program, "sampler"), 0);


	/* Texture */

	glGenTextures(1, &sphere->texture);
	assert(sphere->texture);

	glBindTexture(GL_TEXTURE_2D, sphere->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	sphere->point_idx = -1; /* no texture loaded */


	/* VAO */

	GLuint ebo;

	glGenVertexArrays(1, &sphere->vao);
	glGenBuffers(1, &sphere->vbo);
	glGenBuffers(1, &ebo);
	assert(sphere->vao && sphere->vbo && ebo);

	glBindVertexArray(sphere->vao);

	glBindBuffer(GL_ARRAY_BUFFER, sphere->vbo);

	GLfloat vertices[5 * (ures + 1) * (vres + 1)], *vcur = vertices;
	GLuint indices[2 * ures * (vres + 1)], *icur = indices;

	for (size_t i = 0; i <= ures; ++i) {
		for (size_t j = 0; j <= vres; ++j) {
			float azi_frac = (float) i / ures;
			float inc_frac = (float) j / vres;

			float azi = 2.0f * M_PI * azi_frac;
			float inc = M_PI * inc_frac;

			*(vcur++) = cosf(azi) * sinf(inc); /* x */
			*(vcur++) = cosf(inc); /* y */
			*(vcur++) = sinf(azi) * sinf(inc); /* z */

			*(vcur++) = 0.75 + azi_frac; /* u */
			*(vcur++) = inc_frac; /* v */

			if (i < ures) {
				*(icur++) = i * (vres + 1) + j;
				*(icur++) = (i + 1) * (vres + 1) + j;
			}
		}
	}

	assert((size_t) (vcur - vertices) == 5 * (ures + 1) * (vres + 1));
	assert((size_t) (icur - indices) == 2 * ures * (vres + 1));

	sphere->elements = icur - indices;

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
		GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
		(GLvoid *) (3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
		GL_STATIC_DRAW);
}

static void
node_sphere_render(struct pgrid_node_sphere *sphere, struct pgrid_grid *grid,
		size_t width, size_t height, float fov, vec3 pos, versor rot,
		float interp_scale)
{
	const float aspect_ratio = (float) width / (float) height;

	struct pgrid_point *p = grid->points + grid->rank_zero_idx;
	struct timespec start, end;
	mat4 projection, view, mvp;
	vec3 trans;
	bool wait = false;

	glm_perspective(fov, aspect_ratio, 0.1f, 10.0f, projection);
	glm_quat_mat4(rot, view);
	glm_vec3_sub(p->pos, pos, trans);
	glm_vec3_scale(trans, interp_scale, trans);
	glm_translate(view, trans);
	glm_mat4_mul(projection, view, mvp);

	glUseProgram(sphere->program);
	glUniformMatrix4fv(glGetUniformLocation(sphere->program, "mvp"), 1,
		GL_FALSE, (float *) mvp);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sphere->texture);

	assert(grid->rank_zero_idx <= SIZE_MAX / 2);
	if ((ssize_t) grid->rank_zero_idx != sphere->point_idx) {
		pgrid_log(PGRID_INFO, "Switching to %s @ (%.2f, %.2f, %.2f)",
			p->path, p->pos[0], p->pos[1], p->pos[2]);

		pthread_mutex_lock(&p->mutex);
		if (!p->data) {
			pgrid_log(PGRID_INFO, "Image is not ready, waiting...");
			wait = true;
			clock_gettime(CLOCK_MONOTONIC, &start);
		}
		while (!p->data) {
			pthread_cond_wait(&p->cond, &p->mutex);
		}
		if (wait) {
			clock_gettime(CLOCK_MONOTONIC, &end);
			++grid->metrics.waits;
			grid->metrics.wait_time += timespec_diff(start, end);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, p->width, p->height, 
			0, GL_RGB, GL_UNSIGNED_BYTE, p->data);
		pthread_mutex_unlock(&p->mutex);

		sphere->point_idx = grid->rank_zero_idx;
	}

	glStencilMask(0x00);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

	glBindVertexArray(sphere->vao);
	glDrawElements(GL_TRIANGLE_STRIP, sphere->elements, GL_UNSIGNED_INT, 0);
}

static void
node_sphere_finish(struct pgrid_node_sphere *sphere)
{
	assert(sphere->vao);
	glBindVertexArray(sphere->vao);
	
	assert(sphere->vbo);
	glDeleteBuffers(1, (GLuint *) &sphere->vbo);

	GLint ebo;
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ebo);
	assert(ebo);
	glDeleteBuffers(1, (GLuint *) &ebo);

	glDeleteVertexArrays(1, &sphere->vao);

	assert(sphere->texture);
	glDeleteTextures(1, &sphere->texture);

	assert(sphere->program);
	glDeleteProgram(sphere->program);
}

static void
node_minimap_init(struct pgrid_node_minimap *minimap, struct pgrid_grid *grid)
{	
	/* Program */

	static const GLchar *vs_src = "#version 460 core\n"
		"layout (location = 0) in vec3 pos;\n"
		"uniform mat4 mvp;\n"
		"void main() {\n"
		"        gl_Position = mvp * vec4(pos.xyz, 1.0f);\n"
		"        gl_PointSize = 8;\n"
		"}\n";

	static const GLchar *fs_src = "#version 460 core\n"
		"out vec4 color;\n"
		"uniform vec3 ucolor;\n"
		"void main() {\n"
		"        color = vec4(ucolor.xyz, 1.0f);\n"
		"}\n";

	minimap->program = program_create(vs_src, fs_src);
	glUseProgram(minimap->program);


	/* VAO */

	glGenVertexArrays(1, &minimap->vao);
	glGenBuffers(1, &minimap->vbo);
	assert(minimap->vao && minimap->vbo);

	glBindVertexArray(minimap->vao);

	glBindBuffer(GL_ARRAY_BUFFER, minimap->vbo);

	GLfloat vertices_bg[3 * 10] = {
		1, 1, 0,
		1, -1, 0, 
		-1, -1, 0,
		1, 1, 0,
		-1, 1, 0,
		-1, -1, 0,

		0, -1, 0,
		0, 1, 0,
		-1, 0, 0,
		1, 0, 0,
	};

	GLfloat vertices_pts[3 * grid->points_ln];

	for (size_t i = 0; i < grid->points_ln; ++i) {
		for (size_t j = 0; j < 3; ++j) {
			vertices_pts[3 * i + j] = grid->points[i].pos[j];
		}
	}

	minimap->points = grid->points_ln;

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_bg) + sizeof(vertices_pts),
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_bg), vertices_bg);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices_bg),
		sizeof(vertices_pts), vertices_pts);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(0);
}

static void
node_minimap_render(struct pgrid_node_minimap *minimap, size_t width,
		size_t height, vec3 pos, versor rot)
{
	const float aspect_ratio = (float) width / (float) height;

	mat4 projection, view, mvp;
	
	glm_translate_make(view, (vec3) {0.75f, -0.75f, 0});
	glm_scale(view, (vec3) {0.25f, 0.25f, 1.0f});

	glUseProgram(minimap->program);
	glUniformMatrix4fv(glGetUniformLocation(minimap->program, "mvp"), 1,
		GL_FALSE, (float *) view);

	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);

	glUniform3f(glGetUniformLocation(minimap->program, "ucolor"),
		0.0f, 0.0f, 0.0f);

	glBindVertexArray(minimap->vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glUniform3f(glGetUniformLocation(minimap->program, "ucolor"),
		0.5f, 0.5f, 0.5f);

	glDrawArrays(GL_LINES, 6, 4);

	glm_scale_make(projection, (vec3) {1.0f, aspect_ratio, 1.0f});
	glm_rotate_x(projection, M_PI_2, projection);
	glm_translate_make(view, (vec3) {0.75f, 0.0f, 0.75f / aspect_ratio});
	glm_scale(view, (vec3) {0.125f, 0.125f, 0.125f});
	glm_translate(view, (vec3) {-pos[0], 0.0f, -pos[2]});
	rot[0] = 0;
	rot[2] = 0;
	glm_quat_normalize(rot);
	glm_quat_rotate_at(view, rot, pos);
	glm_mat4_mul(projection, view, mvp);

	glUseProgram(minimap->program);
	glUniformMatrix4fv(glGetUniformLocation(minimap->program, "mvp"), 1,
		GL_FALSE, (float *) mvp);

	glStencilFunc(GL_EQUAL, 1, 0xFF);

	glUniform3f(glGetUniformLocation(minimap->program, "ucolor"),
		0.75f, 0.75f, 0.75f);

	glBindVertexArray(minimap->vao);
	glDrawArrays(GL_POINTS, 10, minimap->points);
}

static void
node_minimap_finish(struct pgrid_node_minimap *minimap)
{
	assert(minimap->vao);
	glBindVertexArray(minimap->vao);
	
	assert(minimap->vbo);
	glDeleteBuffers(1, (GLuint *) &minimap->vbo);

	glDeleteVertexArrays(1, &minimap->vao);

	assert(minimap->program);
	glDeleteProgram(minimap->program);
}

static void
scene_init(struct pgrid_scene *scene, struct pgrid_grid *grid)
{
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	node_sphere_init(&scene->sphere);
	node_minimap_init(&scene->minimap, grid);
}

static void
scene_finish(struct pgrid_scene *scene)
{
	node_sphere_finish(&scene->sphere);
	node_minimap_finish(&scene->minimap);
}

static void
scene_render(struct pgrid *pgrid, vec3 pos, versor rot)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	node_sphere_render(&pgrid->scene.sphere, pgrid->grid, pgrid->width,
		pgrid->height, pgrid->fov, pos, rot, pgrid->interp_scale);

	if (pgrid->minimap) {
		node_minimap_render(&pgrid->scene.minimap, pgrid->width,
			pgrid->height, pos, rot);
	}
}

struct idx_dist {
	size_t idx;
	float dist;
};

static int
idx_dist_compar(const void *a, const void *b)
{
	float da = ((struct idx_dist *)a)->dist;
	float db = ((struct idx_dist *)b)->dist;

	return da < db ? -1 : (da > db ? 1 : 0);
}

static void
grid_rank(struct pgrid_grid *grid, vec3 pos)
{
	struct idx_dist dists[grid->points_ln];
	for (size_t i = 0; i < grid->points_ln; ++i) {
		dists[i].idx = i;
		dists[i].dist = glm_vec3_distance2(pos, grid->points[i].pos);
	}
	qsort(dists, grid->points_ln, sizeof(struct idx_dist), idx_dist_compar);

	pthread_mutex_lock(&grid->mutex);
	for (size_t i = 0; i < grid->points_ln; ++i) {
		grid->points[dists[i].idx].rank = i;
	}
	pthread_cond_broadcast(&grid->cond);
	pthread_mutex_unlock(&grid->mutex);
	grid->rank_zero_idx = dists[0].idx;
	grid->rank_pos[0] = pos[0];
	grid->rank_pos[1] = pos[1];
	grid->rank_pos[2] = pos[2];
}

static unsigned char *
jpeg_data_create(FILE *file, size_t *width, size_t *height)
{
	int sz, w, h, s, c;
	unsigned char *buf = NULL, *data = NULL;
	tjhandle dec;

	assert(!fseek(file, 0, SEEK_END));

	sz = ftell(file);
	assert(sz > 0);

	assert(!fseek(file, 0, SEEK_SET));

	buf = tjAlloc(sz);
	assert(buf);

	assert(fread(buf, sz, 1, file));

	dec = tjInitDecompress();
	assert(dec);
	
	assert(!tjDecompressHeader3(dec, buf, sz, &w, &h, &s, &c));
	assert(w > 0 && h > 0);

	data = tjAlloc(w * h * tjPixelSize[TJPF_RGB]);
	assert(data);

	assert(!tjDecompress2(dec, buf, sz, data, w, 0, h, TJPF_RGB, 0));

	tjFree(buf);

	assert(!tjDestroy(dec));

	*width = w;
	*height = h;

	return data;
}

static void
jpeg_data_destroy(unsigned char *data)
{
	tjFree(data);
}

void
pgrid_point_data_init(struct pgrid_point *point)
{
	/* Make sure path is correctly null terminated */
	char path[point->path_sz + 1];
	memcpy(path, point->path, point->path_sz);
	path[point->path_sz] = '\0';

	FILE *file = fopen(path, "rb");
	assert(file);
	point->data = jpeg_data_create(file, &point->width, &point->height);
	assert(point->data);
	assert(!fclose(file));
}

void
pgrid_point_data_finish(struct pgrid_point *point)
{
	jpeg_data_destroy(point->data);
	point->data = NULL;
}

void
pgrid_point_init(struct pgrid_point *point)
{
	point->path = NULL;
	point->data = NULL;
	point->rank = SIZE_MAX;

	pthread_mutex_init(&point->mutex, NULL);
	pthread_cond_init(&point->cond, NULL);
}

void
pgrid_point_finish(struct pgrid_point *point)
{
	if (point->data) {
		pgrid_point_data_finish(point);
	}
	if (point->path) {
		free(point->path);
		point->path = NULL;
	}

	pthread_mutex_destroy(&point->mutex);
	pthread_cond_destroy(&point->cond);
}

static bool
point_parse(const char *line, size_t line_sz, struct pgrid_point *point)
{
	/* Make sure line is correctly null terminated */
	char l_line[line_sz + 1];
	memcpy(l_line, line, line_sz);
	l_line[line_sz] = '\0';

	char *tok = strtok(l_line, " ");
	if (!tok) {
		return false;
	}
	point->path_sz = strlen(tok) + 1;
	assert(!point->path);
	point->path = malloc(point->path_sz);
	memcpy(point->path, tok, point->path_sz);

	/* TODO: use end pointer in strtof */
	tok = strtok(NULL, " ");
	assert(tok);
	point->pos[0] = strtof(tok, NULL);
	
	tok = strtok(NULL, " ");
	assert(tok);
	point->pos[1] = strtof(tok, NULL);

	tok = strtok(NULL, " ");
	assert(tok);
	point->pos[2] = strtof(tok, NULL);

	point->rot[0] = 0;
	point->rot[1] = 0;
	point->rot[2] = 0;
	point->rot[3] = 0;

	return true;
}

void
pgrid_grid_init(struct pgrid_grid *grid, size_t raw_points)
{
	grid->points = NULL;
	grid->points_ln = 0;
	grid->rank_zero_idx = 0;
	grid->raw_points = raw_points;
	grid->rank_pos[0] = NAN;
	grid->rank_pos[1] = NAN;
	grid->rank_pos[2] = NAN;

	pthread_mutex_init(&grid->mutex, NULL);
	pthread_cond_init(&grid->cond, NULL);
}

void
pgrid_grid_finish(struct pgrid_grid *grid)
{
	if (grid->points) {
		for (size_t i = 0; i < grid->points_ln; ++i) {
			pgrid_point_finish(grid->points + i);
		}
		free(grid->points);
		grid->points = NULL;
	}

	pthread_mutex_destroy(&grid->mutex);
	pthread_cond_destroy(&grid->cond);
}

void
pgrid_grid_load(struct pgrid_grid *grid, const char *path, size_t path_sz)
{
	/* Make sure path is correctly null terminated */
	char lpath[path_sz + 1];
	memcpy(lpath, path, path_sz);
	lpath[path_sz] = '\0';

	assert(!grid->points);

	FILE *file = fopen(path, "r");
	assert(file);

	char *line = NULL;
	size_t line_sz = 0;
	grid->points_ln = 0;

	/* Count valid points */
	struct pgrid_point dummy;
	while (true) {
		ssize_t sz = getline(&line, &line_sz, file);
		if (sz < 1) {
			break;
		}
		pgrid_point_init(&dummy);
		if (point_parse(line, sz + 1, &dummy)) {
			++grid->points_ln;
		}
		pgrid_point_finish(&dummy);
	}
	pgrid_log(PGRID_INFO, "Found %ld points", grid->points_ln);

	assert(!fseek(file, 0, SEEK_SET));

	grid->points = malloc(grid->points_ln * sizeof(struct pgrid_point));

	for (size_t i = 0; i < grid->points_ln; ++i) {
		pgrid_point_init(grid->points + i);
		size_t sz = getline(&line, &line_sz, file);
		assert(sz > 1);
		assert(point_parse(line, sz + 1, grid->points + i));
		pgrid_log(PGRID_DEBUG, "Parsed point: %s @ (%.2f, %.2f, %.2f)",
			grid->points[i].path, grid->points[i].pos[0],
			grid->points[i].pos[1], grid->points[i].pos[2]);
	}

	free(line);

	assert(!fclose(file));
}

void
pgrid_grid_single(struct pgrid_grid *grid, const char *path, size_t path_sz)
{
	assert(!grid->points);

	grid->points_ln = 1;
	grid->points = malloc(sizeof(struct pgrid_point));

	pgrid_point_init(grid->points);
	grid->points[0].path_sz = path_sz;
	grid->points[0].path = malloc(path_sz + 1);
	memcpy(grid->points[0].path, path, path_sz);
	grid->points[0].path[path_sz] = '\0';

	pgrid_point_data_init(grid->points + 0);
}

void
pgrid_init(struct pgrid *pgrid, struct pgrid_grid *grid, size_t width,
		size_t height, float fov)
{
	pgrid->grid = grid;
	pgrid->width = width;
	pgrid->height = height;
	pgrid->fov = fov;
	pgrid->interp_scale = 0.0f;
	pgrid->minimap = false;

	pgrid->metrics.frames = 0;
	pgrid->metrics.frame_time = 0.0;
	pgrid->metrics.max_frame_time = 0.0;

	scene_init(&pgrid->scene, pgrid->grid);
}

void
pgrid_finish(struct pgrid *pgrid)
{
	scene_finish(&pgrid->scene);
}

void
pgrid_render(struct pgrid* pgrid, vec3 pos, versor rot)
{
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	if (pos[0] != pgrid->grid->rank_pos[0]
			|| pos[1] != pgrid->grid->rank_pos[1]
			|| pos[2] != pgrid->grid->rank_pos[2]) {
		grid_rank(pgrid->grid, pos);
	}

	scene_render(pgrid, pos, rot);

	clock_gettime(CLOCK_MONOTONIC, &end);
	++pgrid->metrics.frames;
	double frame_time = timespec_diff(start, end);
	pgrid->metrics.frame_time += frame_time;
	pgrid->metrics.max_frame_time = fmax(pgrid->metrics.max_frame_time,
		frame_time);
}

static void *thread(void *arg)
{
	struct pgrid_grid *grid = arg;
	
	while (grid->raw_points) {
		size_t limit = grid->raw_points;
		bool changed = false;

		for (size_t i = 0; i < grid->points_ln; ++i) {
			struct pgrid_point *p = grid->points + i;
			if (!pthread_mutex_trylock(&p->mutex)) {
				if (p->rank < limit && !p->data) {
					pgrid_point_data_init(p);
					pthread_cond_broadcast(&p->cond);
					++grid->metrics.decoded;
					changed = true;
				} else if (p->rank >= limit && p->data) {
					pgrid_point_data_finish(p);
					++grid->metrics.evicted;
					changed = true;
				}
				pthread_mutex_unlock(&p->mutex);
			}
		}

		/* TODO: not sure if this is enough */
		if (!changed) {
			pthread_mutex_lock(&grid->mutex);
			pthread_cond_wait(&grid->cond, &grid->mutex);
			pthread_mutex_unlock(&grid->mutex);
		}
	}

	return NULL;
}

void
pgrid_threads_init(struct pgrid_grid *grid, pthread_t *threads,
	size_t threads_ln)
{
	for (size_t i = 0; i < threads_ln; ++i) {
		assert(!pthread_create(threads + i, NULL, thread, grid));
	}
}

void
pgrid_threads_finish(struct pgrid_grid *grid, pthread_t *threads,
		size_t threads_ln)
{
	grid->raw_points = 0;
	pthread_cond_broadcast(&grid->cond);

	for (size_t i = 0; i < threads_ln; ++i) {
		assert(!pthread_join(threads[i], NULL));
	}
}

void
pgrid_metrics_print(FILE *file, struct pgrid *pgrid, struct pgrid_grid *grid)
{
	fprintf(file, "Frames rendered: %ld\n", pgrid->metrics.frames);
	fprintf(file, "Average FPS: %lf\n", (double) pgrid->metrics.frames
		/ pgrid->metrics.frame_time);
	fprintf(file, "Min FPS: %lf\n", 1.0 / pgrid->metrics.max_frame_time);
	fprintf(file, "\n");
	fprintf(file, "Wait events: %ld\n", grid->metrics.waits);
	fprintf(file, "Average wait time: %lf s\n", grid->metrics.wait_time
		/ grid->metrics.waits);
	fprintf(file, "\n");
	fprintf(file, "Total decoded: %ld\n", grid->metrics.decoded);
	fprintf(file, "Total evicted: %ld\n", grid->metrics.evicted);
}

void
pgrid_quat_euler(float pitch, float yaw, float roll, versor dest)
{
	float cp = cosf(pitch / 2.0f);
	float sp = sinf(pitch / 2.0f);
	float cy = cosf(yaw / 2.0f);
	float sy = sinf(yaw / 2.0f);
	float cr = cosf(roll / 2.0f);
	float sr = sinf(roll / 2.0f);

	versor tmp;

	dest[0] = 0;
	dest[1] = sy;
	dest[2] = 0;
	dest[3] = cy;

	tmp[0] = 0;
	tmp[1] = 0;
	tmp[2] = sr;
	tmp[3] = cr;
	glm_quat_mul(tmp, dest, dest);

	tmp[0] = sp;
	tmp[1] = 0;
	tmp[2] = 0;
	tmp[3] = cp;
	glm_quat_mul(tmp, dest, dest);
}
