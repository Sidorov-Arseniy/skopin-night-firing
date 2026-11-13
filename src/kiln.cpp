#include "skopin/kiln.hpp"
#include "skopin/measures.hpp"

namespace skopin 
{

Kiln::Kiln() = default;

Cone Kiln::cone() const
{
    return current_cone_;
}

Door Kiln::door() const 
{
    return current_door_;
}

int Kiln::stokes() const 
{
    return current_stokes_;
}

std::vector<Ware> Kiln::stack() const
{
    std::vector<Ware> result;
    result.reserve(wares_.size());
    for (const auto& item : wares_) 
    {
        result.push_back(item.ware);
    }
    return result;
}

void Kiln::open() 
{
    if (current_door_ == Door::Open) 
    {
        throw KilnError("Door is already open");
    }
    if (current_cone_ == Cone::Glaze) 
    {
        throw KilnError("Cannot open door at Glaze cone");
    }
    current_door_ = Door::Open;
}

void Kiln::close() 
{
    if (current_door_ == Door::Closed) 
    {
        throw KilnError("Door is already closed");
    }
    current_door_ = Door::Closed;
}

void Kiln::fire() 
{
    if (current_door_ != Door::Closed) 
    {
        throw KilnError("Cannot fire with an open door");
    }
    if (current_cone_ == Cone::Glaze) 
    {
        throw KilnError("Cannot fire beyond Glaze cone");
    }

    for (const auto& item : wares_) 
    {
        if (item.ware.thickness_mm > kCrackThicknessMm) 
        {
            throw KilnError("Ware is too thick and will crack");
        }
        if (current_cone_ == Cone::Bisque && item.ware.program == Program::Bisque) 
        {
            throw KilnError("Cannot fire Bisque ware to Glaze cone");
        }
    }

    if (current_cone_ == Cone::Cold) 
    {
        current_cone_ = Cone::Bisque;
    } else if (current_cone_ == Cone::Bisque) 
    {
        current_cone_ = Cone::Glaze;
        for (auto& item : wares_) 
        {
            item.reached_glaze = true;
        }
    }
    ++current_stokes_;
}

void Kiln::cool() 
{
    if (current_door_ != Door::Closed) 
    {
        throw KilnError("Cannot cool with an open door");
    }
    if (current_cone_ == Cone::Cold) 
    {
        throw KilnError("Cannot cool below Cold cone");
    }

    if (current_cone_ == Cone::Bisque) 
    {
        for (const auto& item : wares_) 
        
        {
            if (item.ware.program == Program::Glaze && !item.reached_glaze)
            {
                throw KilnError("Cannot cool unglazed Glaze ware to Cold cone");
            }
        }
        current_cone_ = Cone::Cold;
    } 
    else if (current_cone_ == Cone::Glaze) 
    {
        current_cone_ = Cone::Bisque;
    }
    ++current_stokes_;
}

void Kiln::load(const Ware& ware) 
{
    if (current_door_ != Door::Open) 
    {
        throw KilnError("Cannot load with a closed door");
    }
    if (current_cone_ == Cone::Glaze) 
    {
        throw KilnError("Cannot load at Glaze cone");
    }
    if (current_cone_ == Cone::Bisque && ware.program != Program::Glaze) 
    {
        throw KilnError("Only Glaze ware can be loaded at Bisque cone");
    }
    if (validateWare(ware).has_value()) 
    {
        throw KilnError("Invalid ware cannot be loaded");
    }

    for (const auto& item : wares_) 
    {
        if (item.ware.mark == ware.mark) 
        {
            throw KilnError("Duplicate mark in kiln");
        }
        if (item.ware.kind == WareKind::Sagger) 
        {
            throw KilnError("Kiln already contains a sagger");
        }
    }

    if (ware.kind == WareKind::Sagger && !wares_.empty()) 
    {
        throw KilnError("Sagger requires an empty kiln");
    }

    if (!fitsInKiln(stack(), ware)) 
    {
        throw KilnError("Ware does not fit in kiln");
    }

    wares_.push_back(LoadedWare{ware, false});
}

void Kiln::unload(const std::string& mark) 
{
    if (current_door_ != Door::Open) 
    {
        throw KilnError("Cannot unload with a closed door");
    }
    if (wares_.empty()) 
    {
        throw KilnError("Kiln is empty");
    }

    const Ware& top = wares_.back().ware;
    if (top.mark != mark) 
    {
        throw KilnError("Mark does not match top ware");
    }
    if (top.program == Program::Glaze && current_cone_ != Cone::Cold) 
    {
        throw KilnError("Glaze ware can only be unloaded at Cold cone");
    }

    wares_.pop_back();
}

}  // namespace skopin