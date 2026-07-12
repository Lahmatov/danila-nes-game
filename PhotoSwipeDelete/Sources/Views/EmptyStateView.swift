import SwiftUI

struct EmptyStateView: View {
    let hasTrash: Bool
    let onReviewTrash: () -> Void

    var body: some View {
        VStack(spacing: 16) {
            Image(systemName: "checkmark.seal.fill")
                .font(.system(size: 64))
                .foregroundStyle(.green)
            Text("All caught up!")
                .font(.title2.bold())
            Text("You've reviewed every photo in your library.")
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
                .padding(.horizontal, 32)
            if hasTrash {
                Button("Review Photos to Delete", action: onReviewTrash)
                    .buttonStyle(.borderedProminent)
                    .tint(.red)
            }
        }
    }
}
