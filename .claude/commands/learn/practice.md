---
name: "Learn: Practice"
description: "Hands-on practice — Project Mode creates real code files for your IDE, Chat Mode for conceptual discussion"
category: Learning
tags: [learning, practice, tdd, coding]
---

Use the learn-anything-practice skill to handle the user's /learn-practice <concept-name> request.
Follow the workflow defined in the skill:
0. Determine practice mode: Project Mode for coding topics (create real files in .learn/topics/<topic>/exercises/), Chat Mode for conceptual topics
1. Load context: match topic and concept from state.json (single source of truth) → check prerequisites
2. Assess difficulty level based on state.json concept fields (beginner/intermediate/challenge)
3. Project Mode: use Bash to create exercise dir → use Write to create README.md + starter file → tell user to open in IDE
   Chat Mode: generate exercise in chat (background → requirements → code template → hint)
4. Project Mode: use Read to review user's code file → optionally use Bash to run it → compose feedback → Write session file FIRST → echo file content verbatim to conversation + Edit state.json (last_practiced, practice_count, confidence, status) + run render.mjs
   Chat Mode: review code submitted in chat → compose feedback → Write session file FIRST → echo file content verbatim to conversation + Edit state.json + run render.mjs
