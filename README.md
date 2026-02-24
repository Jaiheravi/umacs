# µmacs (micro-macs)

I want to build an editor that is the exact same Emacs setup I use day to day, but without the baggage of the past to allow faster development and higher quality. Elisp is nice but Scheme is better. And I want to see how small and simple this can be.

## Naive Roadmap

This is the high-level plan—it doesn't contain all the difficulties and pain I'll face.

1. DONE—Delete everything from Emacs except the C core.
2. DONE—Gradually add back things needed by the C core to compile.
3. DONE—Continue bringing back files until we have a text 
   editor that doesn't crash on start.
4. Remove dead code.
5. Replace elisp with µlisp, a Scheme dialect for µmacs.
6. Rewrite everything again.
7. Put the new codebase in a new fresh repository and release it under a BSD license instead of GPL.

---

Original Emacs codebase ended here: https://github.com/Jaiheravi/umacs-zero/tree/9176826f41d562f42159b402d8b1d3a676ed99a3

---

## Stats at the time of writing<sup>1</sup>

**Size**: 190.79 MB → 37.38 MB `(-80.4%)`

**Files**: 5518 → 736 `(-86.7%)`

**LOC (source)**: 3,454,588 → 665,182 `(-80.7%)`

<sup>1</sup> Commit SHA: `2c3491a8a8648e97afd8cdd25f119843b7141f70`
