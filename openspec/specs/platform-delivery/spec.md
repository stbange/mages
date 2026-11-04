# platform-delivery Specification

## Purpose
Defines the supported platforms and how the web version is published.

## Requirements

### Requirement: Desktop builds
The game SHALL build with CMake for Linux (GCC/Clang) and Windows (MSVC), using SDL3. The window SHALL scale the 320x200 screen by whole pixels and letterbox the rest.

#### Scenario: Window resize
- **WHEN** the window is resized
- **THEN** the screen keeps its aspect ratio with sharp pixels

### Requirement: Web build
The game SHALL build with Emscripten into a page that runs in desktop and mobile browsers, fills the browser window and accepts touch input.

#### Scenario: Phone in landscape
- **WHEN** the page is opened on a phone in landscape orientation
- **THEN** the game fills the screen height and can be played by touch

### Requirement: GitHub Pages deployment
Every push to the default branch SHALL deploy the web build to GitHub Pages. Every push SHALL build all three platforms in CI.

#### Scenario: Push to main
- **WHEN** a commit is pushed to the default branch and CI passes
- **THEN** the new web build is published to GitHub Pages
