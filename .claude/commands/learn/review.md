---
name: "Learn: Review"
description: "Review learning progress — discover weak spots, get personalized recommendations via spaced repetition"
category: Learning
tags: [learning, review, spaced-repetition]
---

Use the learn-anything-review skill to handle the user's /learn-review [topic-name] request.
Follow the workflow defined in the skill:
1. Select topic (or overview all) — read state.json for each topic
2. Analyze learning data from state.json: mastery heatmap → spaced repetition analysis → concept relationship analysis
3. Generate prioritized recommendations: reinforce → continue → new territory → spaced review
4. If "all" selected, show summary across all topics
Note: This is a read-only workflow — do NOT run render.mjs
