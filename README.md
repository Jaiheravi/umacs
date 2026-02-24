# Emacs-zero

I'm trying to strip down Emacs to the bare minimum and then replace elisp 
with Scheme. I want to see how small and efficient it can get with modern 
practices, and without all the things that are unnecessary for a text editor.
To make this happen, I will temporarily remove compatibility with all platforms 
except macOS on Apple Silicon.

This project is meant to be a proof of concept for a new text editor that 
I'm calling µform (microform) which won't share any of Emacs' source code 
but will be functionally equivalent to it—at least the reasonable parts.

## Naive Roadmap

This is the high-level plan—it doesn't contain all the difficulties and pain I'll face.

1. [DONE] Delete everything from Emacs except the C core.
2. [DONE] Gradually add back things needed by the core to compile and run.
3. Remove dead code to get a clean overview for the different parts of the 
   codebase.
4. Embed a Scheme interpreter.
5. Rewrite all existing elisp code with µlisp.
6. Rewrite the C core and release everything as µform, under a BSD license 
   instead of GPL.

---

Original Emacs codebase ended here: https://github.com/Jaiheravi/umacs-zero/tree/9176826f41d562f42159b402d8b1d3a676ed99a3

---

## Stats at the time of writing<sup>1</sup>

**Size**: 190.79 MB → 37.38 MB `(-80.4%)`

**Files**: 5518 → 736 `(-86.7%)`

**LOC (source)**: 3,454,588 → 665,182 `(-80.7%)`

<sup>1</sup> Commit SHA: `2c3491a8a8648e97afd8cdd25f119843b7141f70`
