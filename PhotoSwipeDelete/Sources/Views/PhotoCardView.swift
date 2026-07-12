import SwiftUI
import Photos

struct PhotoCardView: View {
    let asset: PHAsset
    let library: PhotoLibraryManager
    let isTop: Bool
    let onDecide: (SwipeDecision) -> Void

    @State private var image: UIImage?
    @State private var dragOffset: CGSize = .zero

    private let swipeThreshold: CGFloat = 120

    var body: some View {
        GeometryReader { geo in
            ZStack {
                RoundedRectangle(cornerRadius: 24)
                    .fill(Color(.secondarySystemBackground))

                if let image {
                    Image(uiImage: image)
                        .resizable()
                        .aspectRatio(contentMode: .fill)
                        .frame(width: geo.size.width, height: geo.size.height)
                        .clipShape(RoundedRectangle(cornerRadius: 24))
                } else {
                    ProgressView()
                }

                labelOverlay
            }
            .frame(width: geo.size.width, height: geo.size.height)
            .rotationEffect(.degrees(isTop ? Double(dragOffset.width / 20) : 0))
            .offset(isTop ? dragOffset : .zero)
            .gesture(isTop ? dragGesture(in: geo.size) : nil)
            .task(id: asset.localIdentifier) {
                image = await library.requestImage(
                    for: asset,
                    targetSize: CGSize(width: geo.size.width * 2, height: geo.size.height * 2)
                )
            }
        }
        .clipped()
    }

    @ViewBuilder
    private var labelOverlay: some View {
        if isTop && abs(dragOffset.width) > 20 {
            let isKeep = dragOffset.width > 0
            Text(isKeep ? "KEEP" : "DELETE")
                .font(.system(size: 36, weight: .heavy))
                .padding(.horizontal, 18)
                .padding(.vertical, 8)
                .background((isKeep ? Color.green : Color.red).opacity(0.85))
                .foregroundStyle(.white)
                .clipShape(RoundedRectangle(cornerRadius: 12))
                .rotationEffect(.degrees(isKeep ? -12 : 12))
                .opacity(min(1, Double(abs(dragOffset.width) / swipeThreshold)))
                .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: isKeep ? .topLeading : .topTrailing)
                .padding(24)
        }
    }

    private func dragGesture(in size: CGSize) -> some Gesture {
        DragGesture()
            .onChanged { value in
                dragOffset = value.translation
            }
            .onEnded { value in
                if value.translation.width > swipeThreshold {
                    completeSwipe(decision: .keep, direction: 1, size: size)
                } else if value.translation.width < -swipeThreshold {
                    completeSwipe(decision: .delete, direction: -1, size: size)
                } else {
                    withAnimation(.spring()) {
                        dragOffset = .zero
                    }
                }
            }
    }

    private func completeSwipe(decision: SwipeDecision, direction: CGFloat, size: CGSize) {
        withAnimation(.easeOut(duration: 0.25)) {
            dragOffset = CGSize(width: direction * size.width * 1.5, height: dragOffset.height)
        }
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
            onDecide(decision)
        }
    }
}
