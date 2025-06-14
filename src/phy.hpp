#pragma once

#include "helpers/offset-data-view.hpp"
#include "helpers/text-section-parser.hpp"
#include "structs/phy.hpp"
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <vector>

namespace PhyParser {
  class Phy {
  public:
    struct Solid {
      std::vector<Structs::Vector4> vertices;
      std::vector<uint16_t> indices;
      Structs::Vector3 centreOfMass;
      int32_t boneIndex;
    };

    explicit Phy(const std::span<std::byte>& data, const std::optional<int64_t>& checksum = std::nullopt);

    [[nodiscard]] int64_t getChecksum() const;

    [[nodiscard]] const std::vector<Solid>& getSolids() const;

    [[nodiscard]] const TextSection& getTextSection() const;

  private:
    Structs::Header header;
    std::vector<Solid> solids;
    TextSection textSection;

    [[nodiscard]] static std::vector<Solid> parseSurface(
      const Structs::SurfaceHeader& surfaceHeader, const OffsetDataView& data
    );

    [[nodiscard]] static std::vector<Solid> parseCompactSurface(const OffsetDataView& data);

    [[nodiscard]] static std::vector<Solid> parseMopp(const OffsetDataView& data);

    [[nodiscard]] static Solid parseLedge(
      const Structs::Ledge& ledge, const Structs::Vector3& centreOfMass, const OffsetDataView& data
    );

    [[nodiscard]] static uint16_t remapIndex(
      const Structs::Edge& edge, std::map<uint16_t, uint16_t>& remappedIndices, uint32_t& maxIndex
    );
  };
}
