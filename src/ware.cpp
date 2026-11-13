#include "skopin/ware.hpp"

#include <stdexcept>
namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) 
{
    if (ware.mark == "")
    {
        return "empty mark";
    }
    if (ware.mark.find(" ") != std::string::npos or ware.mark.find("\t") != std::string::npos or ware.mark.find("\n") != std::string::npos){
        return "bad mark";
    }
    if (ware.height_cm <= 0 or ware.height_cm > 48){
        return "bad height";
    }
    if (ware.thickness_mm <= 0){
        return "bad thickness";
    }
    return std::nullopt;
}

bool fitsInKiln(const std::vector<Ware>& wares, const Ware& ware) 
{
    int c = ware.height_cm;
    for (const Ware& w: wares)
    {
        c +=  w.height_cm + 2;
    }
    return c <= 48;
}
} // namespace skopin