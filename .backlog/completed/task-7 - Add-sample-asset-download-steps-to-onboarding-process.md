---
id: TASK-7
title: Add sample asset download steps to onboarding process
status: Done
assignee: []
created_date: '2026-04-07 12:43'
updated_date: '2026-04-07 17:29'
labels:
  - onboarding
  - documentation
  - assets
dependencies: []
references:
  - OriginalAssets/
  - README.md
  - .gitignore
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The engine requires test assets (models and textures) in `OriginalAssets/` which is gitignored. Current assets are collected from open-source/free sites (Sponza PBR, sibenik, fireplace room, lantern, bunny, wolf, etc.) and cannot be distributed directly in the repo. New contributors have no way to know what assets are needed or where to get them.

Need to document:
- Which assets are required for each test scene (UnitTest, GITestBox, GITestSponza_PBR, GITestSibenik, GITestFireplaceRoom)
- Download URLs for each asset (all are from open-source/free sources)
- Expected directory structure under `OriginalAssets/Models/` and `OriginalAssets/Textures/`
- Ideally a download script (`Scripts/DownloadAssets.ps1`) that automates the process
- Update README.md with onboarding steps referencing asset setup

Current asset sources to document:
- Models: Sponza_PBR, sibenik, fireplace_room, lantern, bunny, bob, wolf, orb, sponza
- Textures: PBS materials, Sponza_PBR textures, fireplace_room, lantern, sibenik, skybox, IBL maps
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Scripts/DownloadAssets.ps1 downloads Stanford Bunny, Stanford Dragon, and ShaderBall automatically
- [x] #2 Script prints manual download instructions for Intel Sponza Base and Colorful Curtains
- [x] #3 README.md documents asset sources, directory structure, and download steps
- [x] #4 Dragon import added to f_convertModel in World.inl
- [x] #5 Build passes after changes
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Added asset onboarding pipeline:\n- Scripts/DownloadAssets.ps1 auto-downloads Stanford Bunny, Dragon (PLY), and ShaderBall (FBX, public domain)\n- Script prints manual instructions for Intel Sponza Base and Colorful Curtains (require browser sign-in)\n- README.md updated with asset table, sources, licenses, directory structure, and download instructions\n- Dragon import added to f_convertModel in World.inl\n- Build and RenderTest pass
<!-- SECTION:FINAL_SUMMARY:END -->
