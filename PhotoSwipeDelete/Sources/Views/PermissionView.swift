import SwiftUI

struct PermissionView: View {
    @ObservedObject var library: PhotoLibraryManager

    var body: some View {
        VStack(spacing: 20) {
            Image(systemName: "photo.stack")
                .font(.system(size: 64))
                .foregroundStyle(.pink)
            Text("Swipe Clean")
                .font(.title.bold())
            Text("Swipe left to delete, right to keep. Nothing is removed until you confirm.")
                .multilineTextAlignment(.center)
                .foregroundStyle(.secondary)
                .padding(.horizontal, 32)
            Button {
                Task { await library.requestAuthorization() }
            } label: {
                Text("Allow Photo Access")
                    .bold()
                    .frame(maxWidth: .infinity)
                    .padding()
                    .background(Color.pink)
                    .foregroundStyle(.white)
                    .clipShape(RoundedRectangle(cornerRadius: 14))
            }
            .padding(.horizontal, 32)
        }
    }
}

struct PermissionDeniedView: View {
    var body: some View {
        VStack(spacing: 16) {
            Image(systemName: "lock.slash")
                .font(.system(size: 56))
                .foregroundStyle(.secondary)
            Text("Photo Access Denied")
                .font(.title2.bold())
            Text("Enable photo access in Settings to start cleaning up your library.")
                .multilineTextAlignment(.center)
                .foregroundStyle(.secondary)
                .padding(.horizontal, 32)
            Button("Open Settings") {
                if let url = URL(string: UIApplication.openSettingsURLString) {
                    UIApplication.shared.open(url)
                }
            }
        }
    }
}
