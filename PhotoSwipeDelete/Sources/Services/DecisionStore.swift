import Foundation

/// Remembers which assets the user already chose to keep, so they
/// don't reappear in the deck across app launches.
final class DecisionStore {
    static let shared = DecisionStore()

    private let keptKey = "PhotoSwipeDelete.keptAssetIDs"
    private let defaults = UserDefaults.standard

    private(set) var keptIDs: Set<String>

    private init() {
        keptIDs = Set(defaults.stringArray(forKey: keptKey) ?? [])
    }

    func isKept(_ id: String) -> Bool {
        keptIDs.contains(id)
    }

    func markKept(_ id: String) {
        keptIDs.insert(id)
        persist()
    }

    func unmarkKept(_ id: String) {
        keptIDs.remove(id)
        persist()
    }

    func reset() {
        keptIDs.removeAll()
        persist()
    }

    private func persist() {
        defaults.set(Array(keptIDs), forKey: keptKey)
    }
}
