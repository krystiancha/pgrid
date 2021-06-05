#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "glad/gl.h"
#include "pgrid/pgrid.h"
#include "pgrid/log.h"

struct pgrid_grid grid;
struct pgrid pgrid;
float pitch = 0, yaw = 0, roll = 0;

void
error_callback(int error, const char* description)
{
	(void) error;

	fprintf(stderr, "Error: %s\n", description);
}

void
framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	(void) window;

	glViewport(0, 0, width, height);
	pgrid.width = width;
	pgrid.height = height;
}

void
cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	static const double sensitivity = 0.002;
	static double last_xpos = NAN, last_ypos = NAN;

	if (!isnan(last_xpos) && !isnan(last_ypos)) {
		if (!glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)) {
			pitch += sensitivity * (ypos - last_ypos);
			yaw += sensitivity * (xpos - last_xpos);
		} else {
			roll += sensitivity * (xpos - last_xpos);
		}
	}

	last_xpos = xpos;
	last_ypos = ypos;
}

int
main(int argc, char *argv[])
{
	static const double key_sensitivity = 1;
	static const bool single_mode = false;
	static const char *title_prefix = "Pgrid: ";
	static const int width = 1280;
	static const int height = 720;
	static const float fov = M_PI_2;

	static const struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{"no-vsync", no_argument, NULL, 'n'},
		{"no-minimap", no_argument, NULL, 'm'},
		{"metrics", no_argument, NULL, 's'},
		{"interp-scale", required_argument, NULL, 'p'},
		{"threads", required_argument, NULL, 'j'},
		{"log-level", required_argument, NULL, 'l'},
		{0, 0, 0, 0}
	};

	const char *usage = "Usage: pgrid [options] <input>\n"
		"\n"
		"  -h, --help             Show help message and quit.\n"
		"  -n, --no-vsync         Disable vertical synchronization.\n"
		"  -m, --no-minimap       Disable minimap.\n"
		"  -s, --no-metrics       Disable metrics output.\n"
		"  -p, --interp-scale     Interpolation scale (default: 0.5).\n"
		"  -j, --threads          Number of threads to start.\n"
		"                         (default: 6)\n"
		"  -l, --log-level        Verbosity level (0-5, default: 3).\n"
		"\n";

	bool vsync = true;
	bool minimap = true;
	bool metrics = true;
	float interp_scale = 0.5;
	size_t threads_ln = 6;
	enum pgrid_log_level log_level = PGRID_WARNING;

	while (true) {
		int c = getopt_long(argc, argv, "hnmsp:j:l:", long_options, NULL);
		if (c == -1) {
			break;
		}

		int iarg;
		float farg;
		if (optarg) {
			iarg = strtol(optarg, NULL, 10);
			farg = strtof(optarg, NULL);
		}

		switch (c) {
		case 'h':
			printf(usage);
			exit(EXIT_SUCCESS);
		case 'n':
			vsync = false;
			break;
		case 'm':
			minimap = false;
			break;
		case 's':
			metrics = false;
			break;
		case 'p':
			interp_scale = farg;
			break;
		case 'j':
			if (iarg < 1) {
				pgrid_log(PGRID_ERROR, "Number of threads "
					"must be a positive integer. Falling "
					"back to the default (6).");
				iarg = 6;
			}
			threads_ln = iarg;
			break;
		case 'l':
			if (iarg < 0 || (size_t) iarg >= pgrid_log_levels) {
				pgrid_log(PGRID_ERROR, "Unrecognized log level. "
					"Falling back to the default (3).");
				iarg = 3;	
			}
			log_level = iarg;
			break;
		default:
			fprintf(stderr, usage);
			exit(EXIT_FAILURE);
		}
	}

	if (optind != argc - 1) {
		fprintf(stderr, usage);
		exit(EXIT_FAILURE);
	}

	const char *input_path = argv[optind];
	const size_t input_path_ln = strlen(input_path);
	const size_t title_prefix_ln = strlen(title_prefix);

	pthread_t threads[threads_ln];

	pgrid_log_init(log_level);
	pgrid_grid_init(&grid, 5);
	if (single_mode) {
		pgrid_grid_single(&grid, input_path, strlen(input_path));
	} else {
		pgrid_grid_load(&grid, input_path, strlen(input_path));
		pgrid_threads_init(&grid, threads, threads_ln);
	}

	glfwSetErrorCallback(error_callback);
	assert(glfwInit());

	char title[title_prefix_ln + input_path_ln + 1];
	sprintf(title, "%s%s", title_prefix, input_path);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	assert(window);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

	glfwMakeContextCurrent(window);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(vsync);

	pgrid_init(&pgrid, &grid, width, height, fov);
	if (minimap) {
		pgrid.minimap = true;
	}
	pgrid.interp_scale = interp_scale;

	vec3 pos = { 0 };
	double last_time = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		double now = glfwGetTime();

		versor quat;
		pgrid_quat_euler(pitch, yaw, roll, quat);

		vec3 displac = {
			glfwGetKey(window, GLFW_KEY_D)
			- glfwGetKey(window, GLFW_KEY_A),
			0,
			glfwGetKey(window, GLFW_KEY_S)
			- glfwGetKey(window, GLFW_KEY_W)};
		glm_quat_inv(quat, quat);
		glm_quat_rotatev(quat, displac, displac);
		displac[1] = 0;
		glm_normalize(displac);
		glm_vec3_muladds(displac, (now - last_time) * key_sensitivity,
			pos);

		glm_quat_inv(quat, quat);
		pgrid_render(&pgrid, pos, quat);

		glfwSwapBuffers(window);
		glfwPollEvents();

		last_time = now;
	}

	if (metrics) {
		pgrid_metrics_print(stdout, &pgrid, &grid);
	}

	pgrid_finish(&pgrid);

	glfwDestroyWindow(window);
	glfwTerminate();

	pgrid_threads_finish(&grid, threads, threads_ln);
	pgrid_grid_finish(&grid);

	return 0;
}
