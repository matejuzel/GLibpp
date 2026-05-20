#pragma once

namespace GLibpp::Physics {

    struct BicycleParams {
        BicycleParams(
            float wheelRadius, 
            float wheelBase, 
            float wheelTrack, 
            float maxSteerAngle
        ) 
            : wheelRadius(wheelRadius)
            , wheelBase(wheelBase)
            , wheelTrack(wheelTrack)
            , maxSteerAngle(maxSteerAngle) 
        {}

        float wheelRadius; // polomer kola
        float wheelBase; // rozvor (vzdalenost os predni a zadni napravy)
        float wheelTrack; // rozchod (vzdalenost kol na jedne naprave)
        float maxSteerAngle;
    };

}
