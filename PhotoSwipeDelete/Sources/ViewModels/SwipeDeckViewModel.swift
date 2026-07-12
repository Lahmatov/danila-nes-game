import Foundation
import Photos

@MainActor
final class SwipeDeckViewModel: ObservableObject {
    @Published private(set) var deck: [PHAsset] = []
    @Published private(set) var pendingTrash: [PHAsset] = []
    @Published var isDeleting = false
    @Published var deletionError: String?

    let library: PhotoLibraryManager
    private let store: DecisionStore
    private var history: [SwipeRecord] = []

    var remainingCount: Int { deck.count }
    var pendingTrashCount: Int { pendingTrash.count }
    /// Top of the stack is the last element so swipes are O(1).
    var currentAsset: PHAsset? { deck.last }

    init(library: PhotoLibraryManager, store: DecisionStore = .shared) {
        self.library = library
        self.store = store
        buildDeck()
    }

    func buildDeck() {
        let trashIDs = Set(pendingTrash.map(\.localIdentifier))
        deck = library.assets
            .filter { !store.isKept($0.localIdentifier) && !trashIDs.contains($0.localIdentifier) }
            .reversed()
    }

    func decide(_ decision: SwipeDecision, for asset: PHAsset) {
        guard let index = deck.lastIndex(where: { $0.localIdentifier == asset.localIdentifier }) else { return }
        deck.remove(at: index)
        history.append(SwipeRecord(assetID: asset.localIdentifier, decision: decision))
        switch decision {
        case .keep:
            store.markKept(asset.localIdentifier)
        case .delete:
            pendingTrash.append(asset)
        }
    }

    func undoLast() {
        guard let last = history.popLast() else { return }
        guard let asset = library.assets.first(where: { $0.localIdentifier == last.assetID }) else { return }
        switch last.decision {
        case .keep:
            store.unmarkKept(asset.localIdentifier)
        case .delete:
            pendingTrash.removeAll { $0.localIdentifier == asset.localIdentifier }
        }
        deck.append(asset)
    }

    func removeFromTrash(_ asset: PHAsset) {
        pendingTrash.removeAll { $0.localIdentifier == asset.localIdentifier }
        history.removeAll { $0.assetID == asset.localIdentifier && $0.decision == .delete }
        deck.append(asset)
    }

    func confirmDeletion() async {
        isDeleting = true
        deletionError = nil
        do {
            try await library.delete(assets: pendingTrash)
            let deletedIDs = Set(pendingTrash.map(\.localIdentifier))
            history.removeAll { deletedIDs.contains($0.assetID) }
            pendingTrash.removeAll()
        } catch {
            deletionError = error.localizedDescription
        }
        isDeleting = false
    }
}
