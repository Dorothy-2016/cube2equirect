#include "cube2equirect.h"

using namespace std;

cube2equirect::cube2equirect(SDL_Window *win) {
	mainwindow = win;

	frameCount = 0;
	sprintf(frameIdx, "%06d", frameCount);

	equirectW = 3840;
	equirectH = 1920;
	equirectPixels = (GLubyte*)malloc(4*equirectW*equirectH*sizeof(GLubyte));
}

void cube2equirect::initGL(string dir) {
	SDL_GL_SetSwapInterval(1);

	cubemapDir = dir;
	if (cubemapDir[cubemapDir.length()-1] != '/')
		cubemapDir += "/";

	glViewport(0, 0, equirectW, equirectH);

	initShaders("cube2equirect");
	initBuffers();
	initRenderToTexture();
	initCubeTextures();
}

void cube2equirect::render() {
	glBindFramebuffer(GL_FRAMEBUFFER, equirectFramebuffer);

	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	// left side of cube
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cubeTextures[0]);
	glUniform1i(cubeLeftUniform, 0);

	// right side of cube
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, cubeTextures[1]);
	glUniform1i(cubeRightUniform, 1);

	// bottom side of cube
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, cubeTextures[2]);
	glUniform1i(cubeBottomUniform, 2);

	// top side of cube
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, cubeTextures[3]);
	glUniform1i(cubeTopUniform, 3);

	// back side of cube
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, cubeTextures[4]);
	glUniform1i(cubeBackUniform, 4);

	// front side of cube
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, cubeTextures[5]);
	glUniform1i(cubeFrontUniform, 5);

	glBindVertexArray(vertexArrayObject);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);


	// read rendered image into pixel buffer
	glBindTexture(GL_TEXTURE_2D, equirectTexture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, equirectPixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	// save pixel buffer as image
	saveImagePNG("output/equirect_" + string(frameIdx) + ".png", equirectPixels, equirectW, equirectH);


	frameCount++;
	sprintf(frameIdx, "%06d", frameCount);
	SDL_GL_SwapWindow(mainwindow);
}

bool cube2equirect::hasMoreFrames() {
	string nextImage = cubemapDir + frameIdx + "_left.jpg";

	struct stat info;
	if (stat(nextImage.c_str(), &info) == 0 && !(info.st_mode & S_IFDIR)) {
		return true;
	}
	return false;
}

void cube2equirect::initBuffers() {
	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);

	// vertices
	glGenBuffers(1, &vertexPositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexPositionBuffer);
	GLfloat vertices[] = {
		-1.0, -1.0, -1.0,  // left,  bottom, back
		-1.0,  1.0, -1.0,  // left,  top,    back
		 1.0, -1.0, -1.0,  // right, bottom, back
		 1.0,  1.0, -1.0   // right, top,    back
	};
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(vertexPositionAttribute);
	glVertexAttribPointer(vertexPositionAttribute, 3, GL_FLOAT, false, 0, 0);

	// textures
	glGenBuffers(1, &vertexTextureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexTextureBuffer);
	GLfloat textureCoords[] = {
		-1.0, -1.0,  // left,  bottom
		-1.0,  1.0,  // left,  top
		 1.0, -1.0,  // right, bottom
		 1.0,  1.0   // right, top
	};
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), textureCoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(vertexTextureAttribute);
	glVertexAttribPointer(vertexTextureAttribute, 2, GL_FLOAT, false, 0, 0);

	// faces of triangles
	glGenBuffers(1, &vertexIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexIndexBuffer);
	GLushort vertexIndices[] = {
		 0, 3, 1,
		 3, 0, 2
	};
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLushort), vertexIndices, GL_STATIC_DRAW);

	glBindVertexArray(0);
}

void cube2equirect::initRenderToTexture() {
	glGenFramebuffers(1, &equirectFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, equirectFramebuffer);

	glGenTextures(1, &equirectTexture);
	glBindTexture(GL_TEXTURE_2D, equirectTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, equirectW, equirectH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, equirectTexture, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void cube2equirect::initCubeTextures() {
	glGenTextures(6, cubeTextures);

	loadImage(cubemapDir + "000000_left.jpg",   cubeTextures[0], true);
	loadImage(cubemapDir + "000000_right.jpg",  cubeTextures[1], true);
	loadImage(cubemapDir + "000000_bottom.jpg", cubeTextures[2], true);
	loadImage(cubemapDir + "000000_top.jpg",    cubeTextures[3], true);
	loadImage(cubemapDir + "000000_back.jpg",   cubeTextures[4], true);
	loadImage(cubemapDir + "000000_front.jpg",  cubeTextures[5], true);
}

void cube2equirect::updateCubeTextures() {
	loadImage(cubemapDir + frameIdx + "_left.jpg",   cubeTextures[0], false);
	loadImage(cubemapDir + frameIdx + "_right.jpg",  cubeTextures[1], false);
	loadImage(cubemapDir + frameIdx + "_bottom.jpg", cubeTextures[2], false);
	loadImage(cubemapDir + frameIdx + "_top.jpg",    cubeTextures[3], false);
	loadImage(cubemapDir + frameIdx + "_back.jpg",   cubeTextures[4], false);
	loadImage(cubemapDir + frameIdx + "_front.jpg",  cubeTextures[5], false);
}

void cube2equirect::initShaders(std::string name) {
	string vertSource = readFile("shaders/" + name + ".vert");
	GLint vertexShader = compileShader(vertSource, GL_VERTEX_SHADER);

	string fragSource = readFile("shaders/" + name + ".frag");
	GLint fragmentShader = compileShader(fragSource, GL_FRAGMENT_SHADER);
	
	createShaderProgram(name, vertexShader, fragmentShader);
}

GLint cube2equirect::compileShader(string source, GLint type) {
	GLint status;
	GLint shader = glCreateShader(type);

	const char *srcBytes = source.c_str();
	int srcLength = source.length();
	glShaderSource(shader, 1, &srcBytes, &srcLength);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == 0) {
		GLint length;
		char *info;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		info = SDL_stack_alloc(char, length+1);
		glGetShaderInfoLog(shader, length, NULL, info);
		fprintf(stderr, "Failed to compile shader:\n%s\n", info);
		SDL_stack_free(info);

		return -1;
	}
	else {
		return shader;
	}
}

void cube2equirect::createShaderProgram(string name, GLint vertexShader, GLint fragmentShader) {
	GLint status;
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	glBindAttribLocation(shaderProgram, 0, "aVertexPosition");
	glBindAttribLocation(shaderProgram, 1, "aVertexTextureCoord");
	glBindFragDataLocation(shaderProgram, 0, "FragColor");

	glLinkProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
	if(status == 0) {
		fprintf(stderr, "Unable to initialize the shader program\n");
	}

	// set vertex array
	vertexPositionAttribute = glGetAttribLocation(shaderProgram, "aVertexPosition");
	// set texture coord array
	vertexTextureAttribute = glGetAttribLocation(shaderProgram, "aVertexTextureCoord");
	// set image textures
	cubeLeftUniform   = glGetUniformLocation(shaderProgram, "cubeLeftImage");
	cubeRightUniform  = glGetUniformLocation(shaderProgram, "cubeRightImage");
	cubeBottomUniform = glGetUniformLocation(shaderProgram, "cubeBottomImage");
	cubeTopUniform    = glGetUniformLocation(shaderProgram, "cubeTopImage");
	cubeBackUniform   = glGetUniformLocation(shaderProgram, "cubeBackImage");
	cubeFrontUniform  = glGetUniformLocation(shaderProgram, "cubeFrontImage");

	glUseProgram(shaderProgram);
}

string cube2equirect::readFile(string filename) {
	FILE *f = fopen(filename.c_str(), "rb");

	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *text = (char*)malloc(fsize);
	fread(text, fsize, 1, f);
	fclose(f);

	return string(text, fsize);
}

void cube2equirect::loadImage(string filename, GLuint texture, bool firstTime) {
	SDL_Surface* surface = IMG_Load(filename.c_str());
    if (surface == NULL) {
        printf("Error: \"%s\"\n", SDL_GetError());
        return;
    }

    GLenum format = surface->format->BitsPerPixel == 32 ? GL_RGBA : GL_RGB;

    glBindTexture(GL_TEXTURE_2D, texture);

    if (firstTime) {
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
    
    glBindTexture(GL_TEXTURE_2D, 0);

    SDL_FreeSurface(surface);
}

bool cube2equirect::saveImagePNG(string filename, GLubyte *pixels, int width, int height) {
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
        return false;

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, &info);
        return false;
    }

    FILE *fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        png_destroy_write_struct(&png, &info);
        return false;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_colorp palette = (png_colorp)png_malloc(png, PNG_MAX_PALETTE_LENGTH * sizeof(png_color));
    if (!palette) {
        fclose(fp);
        png_destroy_write_struct(&png, &info);
        return false;
    }
    png_set_PLTE(png, info, palette, PNG_MAX_PALETTE_LENGTH);
    png_write_info(png, info);
    png_set_packing(png);

    png_bytepp rows = (png_bytepp)png_malloc(png, height * sizeof(png_bytep));
    for (int i=0; i<height; i++) {
        rows[i] = (png_bytep)(pixels + i * width * 4);
    }

    png_write_image(png, rows);
    png_write_end(png, info);
    png_free(png, palette);
    png_destroy_write_struct(&png, &info);

    fclose(fp);
    delete[] rows;
    return true;
}