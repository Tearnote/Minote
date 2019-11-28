// Minote - render/scene.h
// Renders the playfield scene to the screen

#ifndef RENDER_SCENE_H
#define RENDER_SCENE_H

void initSceneRenderer(void);
void cleanupSceneRenderer(void);

void updateScene(int newCombo);
void renderScene(void);

#endif //RENDER_SCENE_H
