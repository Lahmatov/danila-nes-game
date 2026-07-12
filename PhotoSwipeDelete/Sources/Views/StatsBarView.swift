import SwiftUI

struct StatsBarView: View {
    let remaining: Int
    let trashCount: Int

    var body: some View {
        HStack {
            Label("\(remaining) left", systemImage: "photo.on.rectangle")
            Spacer()
            Label("\(trashCount) to delete", systemImage: "trash")
                .foregroundStyle(trashCount > 0 ? .red : .secondary)
        }
        .font(.subheadline.weight(.medium))
        .foregroundStyle(.secondary)
        .padding(.horizontal)
        .padding(.top, 8)
    }
}
