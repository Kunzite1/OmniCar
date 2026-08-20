---
name: "Learn: Explain"
description: "Recursively deep-dive into a concept — AI explains, guides thinking, you choose the depth"
category: Learning
tags: [learning, explain, socratic, recursive]
---

Use the learn-anything-explain skill to handle the user's /learn-explain <concept-name> request.
Follow the workflow defined in the skill:
1. Load context: match topic → read state.json (single source of truth, do NOT read knowledge-map.md)
2. Assess user level (beginner/intermediate/advanced) and adjust teaching strategy
3. Compose the full explanation: positioning → core mechanism → analogy → code example → common misconceptions → Socratic check
4. CRITICAL — Write the session file FIRST (./.learn/topics/<topic>/sessions/<domain-slug>/<concept-name>-YYYY-MM-DD.md, where <domain-slug> comes from state.json, matching the user's language), then echo the file content verbatim to the conversation. Also update state.json with Edit (last_explained, explain_count, status, confidence). Then run render.mjs to regenerate knowledge-map.md.
5. Identify sub-topics as recursive entry points (only AFTER saving the session and echoing to conversation)
