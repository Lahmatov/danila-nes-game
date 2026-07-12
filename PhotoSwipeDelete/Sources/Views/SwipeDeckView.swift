import SwiftUI
import Photos

struct SwipeDeckView: View {
    @ObservedObject var library: PhotoLibraryManager
    @StateObject private var viewModel: SwipeDeckViewModel
    @State private var showTrash = false

    init(library: PhotoLibraryManager) {
        self.library = library
        _viewModel = StateObject(wrappedValue: SwipeDeckViewModel(library: library))
    }

    var body: some View {
        NavigationStack {
            VStack(spacing: 0) {
                StatsBarView(
                    remaining: viewModel.remainingCount,
                    trashCount: viewModel.pendingTrashCount
                )

                ZStack {
                    if viewModel.deck.isEmpty {
                        EmptyStateView(hasTrash: viewModel.pendingTrashCount > 0) {
                            showTrash = true
                        }
                    } else {
                        let visible = Array(viewModel.deck.suffix(3))
                        ForEach(Array(visible.enumerated()), id: \.element.localIdentifier) { index, asset in
                            PhotoCardView(
                                asset: asset,
                                library: library,
                                isTop: asset.localIdentifier == viewModel.currentAsset?.localIdentifier
                            ) { decision in
                                viewModel.decide(decision, for: asset)
                            }
                            .scaleEffect(scale(for: index, in: visible.count))
                            .offset(y: offsetY(for: index, in: visible.count))
                            .zIndex(Double(index))
                        }
                    }
                }
                .padding()

                controls
            }
            .navigationTitle("Swipe Clean")
            .toolbar {
                ToolbarItem(placement: .topBarTrailing) {
                    Button {
                        showTrash = true
                    } label: {
                        Label("\(viewModel.pendingTrashCount)", systemImage: "trash")
                    }
                    .disabled(viewModel.pendingTrashCount == 0)
                }
            }
            .sheet(isPresented: $showTrash) {
                TrashReviewView(viewModel: viewModel)
            }
        }
        .onAppear { viewModel.buildDeck() }
    }

    private var controls: some View {
        HStack(spacing: 32) {
            Button {
                viewModel.undoLast()
            } label: {
                Image(systemName: "arrow.uturn.backward.circle.fill")
                    .font(.system(size: 44))
                    .foregroundStyle(.yellow)
            }

            Button {
                if let asset = viewModel.currentAsset {
                    viewModel.decide(.delete, for: asset)
                }
            } label: {
                Image(systemName: "xmark.circle.fill")
                    .font(.system(size: 56))
                    .foregroundStyle(.red)
            }
            .disabled(viewModel.currentAsset == nil)

            Button {
                if let asset = viewModel.currentAsset {
                    viewModel.decide(.keep, for: asset)
                }
            } label: {
                Image(systemName: "heart.circle.fill")
                    .font(.system(size: 56))
                    .foregroundStyle(.green)
            }
            .disabled(viewModel.currentAsset == nil)
        }
        .padding(.bottom, 24)
    }

    /// `index` is the position within the visible slice; the last (highest) index is the top card.
    private func scale(for index: Int, in count: Int) -> CGFloat {
        1.0 - (CGFloat(count - 1 - index) * 0.04)
    }

    private func offsetY(for index: Int, in count: Int) -> CGFloat {
        CGFloat(count - 1 - index) * 8
    }
}
