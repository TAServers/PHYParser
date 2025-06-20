#include "phy.hpp"
#include "helpers/offset-data-view.hpp"
#include <algorithm>
#include <ranges>
#include <stack>

namespace PhyParser {
  using namespace Errors;
  using namespace Enums;
  using namespace Structs;

  Phy::Phy(const std::span<std::byte>& data, const std::optional<int64_t>& checksum) {
    const OffsetDataView dataView(data);
    header = dataView.parseStruct<Header>(0, "Failed to parse PHY header").first;

    if (checksum.has_value() && header.checksum != checksum.value()) {
      throw InvalidChecksum("PHY checksum does not match");
    }

    solids.reserve(header.solidCount);
    size_t offset = header.size;
    for (int i = 0; i < header.solidCount; i++) {
      const auto surfaceHeader = dataView.parseStruct<SurfaceHeader>(offset, "Failed to parse surface header").first;

      auto solidsForSurface = parseSurface(surfaceHeader, dataView.withOffset(offset + sizeof(SurfaceHeader)));
      solids.insert(
        solids.end(), std::make_move_iterator(solidsForSurface.begin()), std::make_move_iterator(solidsForSurface.end())
      );

      offset += surfaceHeader.size + sizeof(SurfaceHeader::size);
    }

    const auto rawTextSection = dataView.parseString(offset, "Failed to parse text section");
    textSection = parseTextSection(rawTextSection);
  }

  int64_t Phy::getChecksum() const {
    return header.checksum;
  }

  const std::vector<Phy::Solid>& Phy::getSolids() const {
    return solids;
  }
  const TextSection& Phy::getTextSection() const {
    return textSection;
  }

  std::vector<Phy::Solid> Phy::parseSurface(const SurfaceHeader& surfaceHeader, const OffsetDataView& data) {
    switch (surfaceHeader.modelType) {
      case ModelType::IVPCompactSurface:
        return parseCompactSurface(data);
      case ModelType::IVPMOPP:
        return parseMopp(data);
      case ModelType::IVPBall:
        throw InvalidBody("Unsupported surface model type IVPBall");
      case ModelType::IVPVirtual:
        throw InvalidBody("Unsupported surface model type IVPVirtual");
      default:
        throw InvalidBody("Unrecognised surface model type");
    }
  }

  std::vector<Phy::Solid> Phy::parseCompactSurface(const OffsetDataView& data) {
    const auto [surfaceHeader, headerOffset] =
      data.parseStruct<CompactSurfaceHeader>(0, "Failed to parse compact surface header");

    const auto rootNode = headerOffset + offsetof(CompactSurfaceHeader, massCentre) + surfaceHeader.offsetLedgetreeRoot;

    const auto nodeData = data.withOffset(0);

    std::vector<Solid> solids;
    std::stack<size_t> nodeOffsets;
    nodeOffsets.push(rootNode);

    while (!nodeOffsets.empty()) {
      auto [node, nodeOffset] = nodeData.parseStruct<LedgeNode>(nodeOffsets.top(), "Failed to parse ledge node");
      nodeOffsets.pop();

      if (node.isTerminal()) {
        const auto [ledge, ledgeOffset] =
          nodeData.parseStruct<Ledge>(nodeOffset + node.compactNodeOffset, "Failed to parse ledge");

        solids.push_back(parseLedge(ledge, surfaceHeader.massCentre, data.withOffset(ledgeOffset)));
      } else {
        nodeOffsets.push(nodeOffset + node.rightNodeOffset);
        nodeOffsets.push(nodeOffset + sizeof(LedgeNode));
      }
    }

    return std::move(solids);
  }

  std::vector<Phy::Solid> Phy::parseMopp(const OffsetDataView& /*data*/) {
    throw std::runtime_error("Not implemented");
  }

  Phy::Solid Phy::parseLedge(const Ledge& ledge, const Structs::Vector3& centreOfMass, const OffsetDataView& data) {
    const auto triangles = data.parseStructArrayWithoutOffsets<CompactTriangle>(
      sizeof(Ledge), ledge.trianglesCount, "Failed to parse triangle array"
    );

    std::vector<uint16_t> indices;
    indices.reserve(static_cast<size_t>(ledge.trianglesCount) * 3);
    std::map<uint16_t, uint16_t> remappedIndices;

    uint32_t maxVertexIndex = 0;
    for (const auto& triangle : triangles) {
      for (auto edgeIndex = 0; edgeIndex < 3; edgeIndex++) {
        indices.push_back(remapIndex(triangle.edges[edgeIndex], remappedIndices, maxVertexIndex));
      }
    }

    std::vector<Vector4> vertices;
    vertices.reserve(remappedIndices.size());
    const auto sharedVertexBuffer = data.parseStructArrayWithoutOffsets<Vector4>(
      ledge.pointOffset, maxVertexIndex + 1, "Failed to parse vertex array"
    );

    for (const auto sourceIndex : remappedIndices | std::views::keys) {
      vertices.push_back(sharedVertexBuffer[sourceIndex]);
    }

    return {
      .vertices = std::move(vertices),
      .indices = std::move(indices),
      .centreOfMass = centreOfMass,
      .boneIndex = ledge.boneIndex,
    };
  }

  uint16_t Phy::remapIndex(const Edge& edge, std::map<uint16_t, uint16_t>& remappedIndices, uint32_t& maxIndex) {
    const auto index = edge.getStartPointIndex();
    if (remappedIndices.contains(index)) {
      return remappedIndices.at(index);
    }

    maxIndex = std::max(maxIndex, static_cast<uint32_t>(index));

    const auto remappedIndex = remappedIndices.size();
    remappedIndices.emplace(index, remappedIndex);

    return remappedIndex;
  }
}
