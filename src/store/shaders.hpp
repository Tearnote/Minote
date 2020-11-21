/**
 * Storage for all available shaders
 * @file
 */

#pragma once

#include "sys/opengl/shader.hpp"

namespace minote {

struct Shaders {

	struct Blit : Shader {

		Sampler<Texture> image;
		Uniform<float> boost;

		void setLocations() override
		{
			image.setLocation(*this, "image");
			boost.setLocation(*this, "boost");
		}
	} blit;

	struct Delinearize : Shader {

		Sampler<Texture> image;

		void setLocations() override
		{
			image.setLocation(*this, "image");
		}

	} delinearize;

	struct Threshold : Shader {

		Sampler<Texture> image;
		Uniform<float> threshold;
		Uniform<float> softKnee;
		Uniform<float> strength;

		void setLocations() override
		{
			image.setLocation(*this, "image");
			threshold.setLocation(*this, "threshold");
			softKnee.setLocation(*this, "softKnee");
			strength.setLocation(*this, "strength");
		}

	} threshold;

	struct BoxBlur : Shader {

		Sampler<Texture> image;
		Uniform<float> step;
		Uniform<vec2> imageTexel;

		void setLocations() override
		{
			image.setLocation(*this, "image");
			step.setLocation(*this, "step");
			imageTexel.setLocation(*this, "imageTexel");
		}

	} boxBlur;

	struct SmaaEdge : Shader {

		Sampler<Texture> image;
		Uniform<vec4> screenSize;

		void setLocations() override
		{
			image.setLocation(*this, "image");
			screenSize.setLocation(*this, "screenSize");
		}

	} smaaEdge;

	struct SmaaBlend : Shader {

		Sampler<Texture> edges;
		Sampler<Texture> area;
		Sampler<Texture> search;
		Uniform<vec4> subsampleIndices;
		Uniform<vec4> screenSize;

		void setLocations() override
		{
			edges.setLocation(*this, "edges", TextureUnit::_0);
			area.setLocation(*this, "area", TextureUnit::_1);
			search.setLocation(*this, "search", TextureUnit::_2);
			subsampleIndices.setLocation(*this, "subsampleIndices");
			screenSize.setLocation(*this, "screenSize");
		}

	} smaaBlend;

	struct SmaaNeighbor : Shader {

		Sampler<Texture> image;
		Sampler<Texture> blend;
		Uniform<float> alpha;
		Uniform<vec4> screenSize;

		void setLocations() override
		{
			image.setLocation(*this, "image", TextureUnit::_0);
			blend.setLocation(*this, "blend", TextureUnit::_2);
			alpha.setLocation(*this, "alpha");
			screenSize.setLocation(*this, "screenSize");
		}

	} smaaNeighbor;

	struct Flat : Shader {

		Uniform<mat4> view;
		Uniform<mat4> projection;

		void setLocations() override
		{
			view.setLocation(*this, "view");
			projection.setLocation(*this, "projection");
		}

	} flat;

	struct Phong : Shader {

		Uniform<mat4> view;
		Uniform<mat4> projection;
		Uniform<vec3> lightPosition;
		Uniform<vec3> lightColor;
		Uniform<vec3> ambientColor;
		Uniform<float> ambient;
		Uniform<float> diffuse;
		Uniform<float> specular;
		Uniform<float> shine;

		void setLocations() override
		{
			view.setLocation(*this, "view");
			projection.setLocation(*this, "projection");
			lightPosition.setLocation(*this, "lightPosition");
			lightColor.setLocation(*this, "lightColor");
			ambientColor.setLocation(*this, "ambientColor");
			ambient.setLocation(*this, "ambient");
			diffuse.setLocation(*this, "diffuse");
			specular.setLocation(*this, "specular");
			shine.setLocation(*this, "shine");
		}

	} phong;

	struct Nuklear : Shader {

		Sampler<Texture> atlas;
		Uniform<mat4> projection;

		void setLocations() override
		{
			atlas.setLocation(*this, "atlas");
			projection.setLocation(*this, "projection");
		}

	} nuklear;

	struct Msdf : Shader {

		BufferSampler transforms;
		Sampler<Texture> atlas;
		Uniform<mat4> view;
		Uniform<mat4> projection;

		void setLocations() override
		{
			atlas.setLocation(*this, "atlas", TextureUnit::_0);
			transforms.setLocation(*this, "transforms", TextureUnit::_1);
			projection.setLocation(*this, "projection");
			view.setLocation(*this, "view");
		}

	} msdf;

	void create();

	void destroy();

};

}
