#ifndef RIGHTS_CONSUMER_HPP
#define RIGHTS_CONSUMER_HPP
#include <RightsParser.hpp>
#include <common.hpp>

namespace android {
    class RightsConsumer {

    private:
        time64_t startTime;
        time64_t intervalStartTime;
        time64_t endTime;
        time64_t interval;
        bool needConsume;
        void consumeForAction(RightsParser& rightsParser, int action);
    public:
        RightsConsumer();
        void setPlaybackStatus(int status);
        bool consume(RightsParser& rightsParser);
        bool shouldConsume();
        virtual ~RightsConsumer();
    };

};

#endif  // RIGHTS_CONSUMER_HPP
