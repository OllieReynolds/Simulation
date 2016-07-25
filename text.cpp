#include "text.hpp"

namespace graphics {

	void Text::init_text(int pixelSize) {
		FT_Library ft;
		if (FT_Init_FreeType(&ft))  {
			std::cout << "Error initialising FreeType" << std::endl;
		}

		FT_Face face;
		if (FT_New_Face(ft, "OpenSans-Regular.ttf", 0, &face)) {
			std::cout << "Error initialising font" << std::endl;
		}

		FT_Set_Pixel_Sizes(face, 0, pixelSize);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		for (GLubyte c = 0; c < 128; ++c) {
			if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
				std::cout << "Error retrieving glyph: " << c << std::endl;
			}

			GLuint tex;
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
			);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			Glyph g = {
				tex,
				face->glyph->advance.x,
				maths::vec2i(face->glyph->bitmap.width, face->glyph->bitmap.rows),
				maths::vec2i(face->glyph->bitmap_left, face->glyph->bitmap_top),
			};

			glyph_map.insert(std::pair<GLchar, Glyph>(c, g));
		}

		FT_Done_Face(face);
		FT_Done_FreeType(ft);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
		glEnableVertexAttribArray(0);

		shader = {
			"vs_text.glsl",
			"fs_text.glsl"
		};

		glUniformMatrix4fv(
			glGetUniformLocation(shader.program, "proj"),
			1,
			GL_FALSE,
			&maths::orthogonal_perspective(
				utils::resolution()[0],
				utils::resolution()[1],
				-1.f,
				1.f
			)[0][0]
		);

		glUniform3fv(
			glGetUniformLocation(shader.program, "colour"),
			1,
			&maths::vec3(1.f, 1.f, 0.f)[0]
		);
	}

	void Text::draw_text(const std::string& msg, const maths::vec2f& pos) {
		shader.use();

		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(vao);

		float x = pos[0];
		float y = pos[1];

		std::string::const_iterator c;
		for (c = msg.begin(); c != msg.end(); ++c) {
			Glyph g = glyph_map[*c];

			float xPos = x + g.bearing_offset[0];
			float yPos = y - (g.glyph_size[1] - g.bearing_offset[1]);

			float w = g.glyph_size[0];
			float h = g.glyph_size[1];
			float vertices[6][4] = {
				{xPos,     yPos + h, 0.f, 0.f},
				{xPos,     yPos,     0.f, 1.f},
				{xPos + w, yPos,     1.f, 1.f},
				{xPos,     yPos + h, 0.f, 0.f},
				{xPos + w, yPos,     1.f, 1.f},
				{xPos + w, yPos + h, 1.f, 0.f}
			};

			glBindTexture(GL_TEXTURE_2D, g.data);

			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			x += (g.next_glyph_offset >> 6);
		}

		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Text::destroy_text() {
		glDeleteProgram(shader.program);
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
	}
}