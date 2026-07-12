# Swipe Clean — Tinder-style photo cleanup

An iOS app that lets you go through your photo library one picture at a time,
Tinder-style: swipe right to keep, swipe left to send to trash. Nothing is
deleted from your library until you review the trash pile and confirm — one
tap deletes everything you swiped left, with a single system confirmation.

## Features

- Card-stack swipe UI (drag, tap ❤️/✕, or undo the last decision)
- Full-resolution photo library access via the `Photos` framework
- Photos you "keep" are remembered (`UserDefaults`) so they don't reappear
  next time you open the app
- Deletions are staged in a review screen — you can restore any photo before
  confirming, and the actual delete is a single batched
  `PHPhotoLibrary` change (one system prompt for the whole batch)

## Project layout

```
PhotoSwipeDelete/
├── project.yml              # XcodeGen project spec
└── Sources/
    ├── Info.plist            # Photo library usage description
    ├── App/                  # @main entry point
    ├── Models/                # SwipeDecision, SwipeRecord
    ├── Services/              # PhotoLibraryManager, DecisionStore
    ├── ViewModels/             # SwipeDeckViewModel
    └── Views/                  # ContentView, SwipeDeckView, PhotoCardView, TrashReviewView, ...
```

There's no `.xcodeproj` checked in — this was written outside of Xcode/macOS,
so the project file is generated locally with
[XcodeGen](https://github.com/yonaskolb/XcodeGen) from `project.yml`.

## Build & run (on a Mac with Xcode)

```bash
brew install xcodegen        # once, if you don't have it
cd PhotoSwipeDelete
xcodegen generate
open PhotoSwipeDelete.xcodeproj
```

Then in Xcode:

1. Select the `PhotoSwipeDelete` target → **Signing & Capabilities** → pick
   your team (needed to run on a real device; the simulator works without one).
2. Run on a device or simulator with iOS 17+.
3. On a simulator, add a few photos to the simulated Photos library first
   (drag image files onto the running simulator window) so there's something
   to swipe through.

## Notes / possible next steps

- Currently pulls all photos, newest first. Could add album/date filters.
- Videos aren't included (`PHAsset` fetch is filtered to `.image`) — easy to
  extend to `.video` if wanted.
- No iCloud "Shared Album" support; regular library + iCloud Photos only.
