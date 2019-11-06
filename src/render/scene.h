// Minote - scenerender.h
// Renders the playfield scene to the screen

#ifndef SCENE_H
#define SCENE_H

void initSceneRenderer(void);
void cleanupSceneRenderer(void);

void updateScene(int newCombo);
void renderScene(void);

#endif //SCENE_H