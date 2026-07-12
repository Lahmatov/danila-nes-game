import Foundation
import Photos
import UIKit

@MainActor
final class PhotoLibraryManager: ObservableObject {
    enum AuthState {
        case notDetermined
        case restricted
        case denied
        case limited
        case authorized
    }

    @Published private(set) var authState: AuthState = .notDetermined
    @Published private(set) var assets: [PHAsset] = []

    private let imageManager = PHCachingImageManager()

    func refreshAuthorizationStatus() {
        authState = Self.map(PHPhotoLibrary.authorizationStatus(for: .readWrite))
    }

    func requestAuthorization() async {
        let status = await PHPhotoLibrary.requestAuthorization(for: .readWrite)
        authState = Self.map(status)
        if authState == .authorized || authState == .limited {
            loadAssets()
        }
    }

    private static func map(_ status: PHAuthorizationStatus) -> AuthState {
        switch status {
        case .notDetermined: return .notDetermined
        case .restricted: return .restricted
        case .denied: return .denied
        case .limited: return .limited
        case .authorized: return .authorized
        @unknown default: return .denied
        }
    }

    func loadAssets() {
        let options = PHFetchOptions()
        options.sortDescriptors = [NSSortDescriptor(key: "creationDate", ascending: false)]
        let fetchResult = PHAsset.fetchAssets(with: .image, options: options)
        var result: [PHAsset] = []
        result.reserveCapacity(fetchResult.count)
        fetchResult.enumerateObjects { asset, _, _ in
            result.append(asset)
        }
        assets = result
    }

    func requestImage(for asset: PHAsset, targetSize: CGSize) async -> UIImage? {
        await withCheckedContinuation { continuation in
            let options = PHImageRequestOptions()
            options.deliveryMode = .highQualityFormat
            options.isNetworkAccessAllowed = true
            options.resizeMode = .exact
            imageManager.requestImage(
                for: asset,
                targetSize: targetSize,
                contentMode: .aspectFill,
                options: options
            ) { image, _ in
                continuation.resume(returning: image)
            }
        }
    }

    /// Deletes the given assets in a single system confirmation prompt.
    func delete(assets assetsToDelete: [PHAsset]) async throws {
        guard !assetsToDelete.isEmpty else { return }
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            PHPhotoLibrary.shared().performChanges {
                PHAssetChangeRequest.deleteAssets(assetsToDelete as NSArray)
            } completionHandler: { success, error in
                if success {
                    continuation.resume(returning: ())
                } else {
                    continuation.resume(throwing: error ?? NSError(
                        domain: "PhotoLibraryManager",
                        code: -1,
                        userInfo: [NSLocalizedDescriptionKey: "Unknown error while deleting photos."]
                    ))
                }
            }
        }
        let deletedIDs = Set(assetsToDelete.map(\.localIdentifier))
        assets.removeAll { deletedIDs.contains($0.localIdentifier) }
    }
}
