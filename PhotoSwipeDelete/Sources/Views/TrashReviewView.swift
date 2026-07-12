import SwiftUI
import Photos

struct TrashReviewView: View {
    @ObservedObject var viewModel: SwipeDeckViewModel
    @Environment(\.dismiss) private var dismiss

    private var showError: Binding<Bool> {
        Binding(
            get: { viewModel.deletionError != nil },
            set: { if !$0 { viewModel.deletionError = nil } }
        )
    }

    var body: some View {
        NavigationStack {
            Group {
                if viewModel.pendingTrash.isEmpty {
                    ContentUnavailableView(
                        "Nothing to Delete",
                        systemImage: "trash.slash",
                        description: Text("Photos you swipe left will show up here for review.")
                    )
                } else {
                    List {
                        ForEach(viewModel.pendingTrash, id: \.localIdentifier) { asset in
                            TrashRow(asset: asset, library: viewModel.library) {
                                viewModel.removeFromTrash(asset)
                            }
                        }
                    }
                }
            }
            .navigationTitle("Review (\(viewModel.pendingTrashCount))")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Close") { dismiss() }
                }
                ToolbarItem(placement: .confirmationAction) {
                    Button(role: .destructive) {
                        Task {
                            await viewModel.confirmDeletion()
                            if viewModel.deletionError == nil {
                                dismiss()
                            }
                        }
                    } label: {
                        if viewModel.isDeleting {
                            ProgressView()
                        } else {
                            Text("Delete All")
                        }
                    }
                    .disabled(viewModel.pendingTrash.isEmpty || viewModel.isDeleting)
                }
            }
            .alert("Couldn't Delete Photos", isPresented: showError) {
                Button("OK") { viewModel.deletionError = nil }
            } message: {
                Text(viewModel.deletionError ?? "")
            }
        }
    }
}

private struct TrashRow: View {
    let asset: PHAsset
    let library: PhotoLibraryManager
    let onRestore: () -> Void

    @State private var image: UIImage?

    var body: some View {
        HStack {
            Group {
                if let image {
                    Image(uiImage: image)
                        .resizable()
                        .aspectRatio(contentMode: .fill)
                } else {
                    Color(.secondarySystemBackground)
                }
            }
            .frame(width: 56, height: 56)
            .clipShape(RoundedRectangle(cornerRadius: 8))

            Text(asset.creationDate?.formatted(date: .abbreviated, time: .shortened) ?? "Unknown date")
                .font(.subheadline)

            Spacer()

            Button("Restore", action: onRestore)
                .font(.caption.bold())
                .buttonStyle(.bordered)
        }
        .task(id: asset.localIdentifier) {
            image = await library.requestImage(for: asset, targetSize: CGSize(width: 112, height: 112))
        }
    }
}
