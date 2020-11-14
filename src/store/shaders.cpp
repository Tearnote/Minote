/**
 * Implementation of store/shaders.hpp
 * @file
 */

#include "store/shaders.hpp"

namespace minote {

static constexpr GLchar BlitVert[] = {
#include "blit.vert"
	'\0'};
static constexpr GLchar BlitFrag[] = {
#include "blit.frag"
	'\0'};

static constexpr GLchar DelinearizeVert[] = {
#include "delinearize.vert"
	'\0'};
static constexpr GLchar DelinearizeFrag[] = {
#include "delinearize.frag"
	'\0'};

static constexpr GLchar ThresholdVert[] = {
#include "threshold.vert"
	'\0'};
static constexpr GLchar ThresholdFrag[] = {
#include "threshold.frag"
	'\0'};

static constexpr GLchar BoxBlurVert[] = {
#include "boxBlur.vert"
	'\0'};
static constexpr GLchar BoxBlurFrag[] = {
#include "boxBlur.frag"
	'\0'};

static constexpr GLchar SmaaEdgeVert[] = {
#include "smaaEdge.vert"
	'\0'};
static constexpr GLchar SmaaEdgeFrag[] = {
#include "smaaEdge.frag"
	'\0'};

static constexpr GLchar SmaaBlendVert[] = {
#include "smaaBlend.vert"
	'\0'};
static constexpr GLchar SmaaBlendFrag[] = {
#include "smaaBlend.frag"
	'\0'};

static constexpr GLchar SmaaNeighborVert[] = {
#include "smaaNeighbor.vert"
	'\0'};
static constexpr GLchar SmaaNeighborFrag[] = {
#include "smaaNeighbor.frag"
	'\0'};

static constexpr GLchar FlatVert[] = {
#include "flat.vert"
	'\0'};
static constexpr GLchar FlatFrag[] = {
#include "flat.frag"
	'\0'};

static constexpr GLchar PhongVert[] = {
#include "phong.vert"
	'\0'};
static constexpr GLchar PhongFrag[] = {
#include "phong.frag"
	'\0'};

static constexpr GLchar NuklearVert[] = {
#include "nuklear.vert"
	'\0'};
static constexpr GLchar NuklearFrag[] = {
#include "nuklear.frag"
	'\0'};

static constexpr GLchar MsdfVert[] = {
#include "msdf.vert"
	'\0'};
static constexpr GLchar MsdfFrag[] = {
#include "msdf.frag"
	'\0'};

void Shaders::create()
{
	blit.create("blit", BlitVert, BlitFrag);
	delinearize.create("delinearize", DelinearizeVert, DelinearizeFrag);
	threshold.create("threshold", ThresholdVert, ThresholdFrag);
	boxBlur.create("boxBlur", BoxBlurVert, BoxBlurFrag);
	smaaEdge.create("smaaEdge", SmaaEdgeVert, SmaaEdgeFrag);
	smaaBlend.create("smaaBlend", SmaaBlendVert, SmaaBlendFrag);
	smaaNeighbor.create("smaaNeighbor", SmaaNeighborVert, SmaaNeighborFrag);
	flat.create("flat", FlatVert, FlatFrag);
	phong.create("phong", PhongVert, PhongFrag);
	nuklear.create("nuklear", NuklearVert, NuklearFrag);
	msdf.create("msdf", MsdfVert, MsdfFrag);
}

void Shaders::destroy()
{
	blit.destroy();
	delinearize.destroy();
	threshold.destroy();
	boxBlur.destroy();
	smaaEdge.destroy();
	smaaBlend.destroy();
	smaaNeighbor.destroy();
	flat.destroy();
	phong.destroy();
	nuklear.destroy();
	msdf.destroy();
}

}
