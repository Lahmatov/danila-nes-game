import SwiftUI

struct ContentView: View {
    @StateObject private var library = PhotoLibraryManager()

    var body: some View {
        Group {
            switch library.authState {
            case .notDetermined:
                PermissionView(library: library)
            case .denied, .restricted:
                PermissionDeniedView()
            case .authorized, .limited:
                SwipeDeckView(library: library)
            }
        }
        .onAppear {
            library.refreshAuthorizationStatus()
            if library.authState == .authorized || library.authState == .limited {
                library.loadAssets()
            }
        }
    }
}
