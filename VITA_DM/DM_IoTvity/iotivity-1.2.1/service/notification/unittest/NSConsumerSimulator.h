//******************************************************************
//
// Copyright 2016 Samsung Electronics All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef _NS_CONSUMER_SIMULATOR_H_
#define _NS_CONSUMER_SIMULATOR_H_

#include <iostream>

#include "OCPlatform.h"
#include "OCApi.h"

class NSConsumerSimulator
{
private:
    std::function<void(const int&, const std::string&, const std::string&,
            const std::string&)> m_messageFunc;
    std::function<void(const int&, const int&)> m_syncFunc;

    std::shared_ptr<OC::OCResource> m_syncResource;
    std::shared_ptr<OC::OCResource> m_msgResource;
    std::shared_ptr<OC::OCResource> m_topicResource;

    bool isTopicPost;

public:
    NSConsumerSimulator()
    : m_messageFunc(), m_syncFunc(),
      m_syncResource(), isTopicPost(false) { };
    ~NSConsumerSimulator() = default;

    NSConsumerSimulator(const NSConsumerSimulator &) = delete;
    NSConsumerSimulator & operator = (const NSConsumerSimulator &) = delete;

    NSConsumerSimulator(NSConsumerSimulator &&) = delete;
    NSConsumerSimulator & operator = (NSConsumerSimulator &&) = delete;

    void findProvider()
    {
        OC::OCPlatform::findResource("", std::string("/oic/res?rt=oic.wk.notification"),
                OCConnectivityType::CT_DEFAULT,
                std::bind(&NSConsumerSimulator::findResultCallback, this, std::placeholders::_1),
                OC::QualityOfService::LowQos);
    }

    void syncToProvider(int & type, const int & id, const std::string & providerID)
    {
        if (m_syncResource == nullptr)
        {
            return;
        }

        OC::OCRepresentation rep;
        rep.setValue("providerid", providerID);
        rep.setValue("messageid", id);
        rep.setValue("state", type);

        m_syncResource->post(rep, OC::QueryParamsMap(), &onPost, OC::QualityOfService::LowQos);
    }

    bool cancelObserves()
    {
        if(!msgResourceCancelObserve(OC::QualityOfService::HighQos) &&
                !syncResourceCancelObserve(OC::QualityOfService::HighQos))
            return true;
        return false;
    }

    void setCallback(std::function<void(const int&, const std::string&,
            const std::string&, const std::string&)> messageFunc,
            const std::function<void(const int&, const int&)> & syncFunc)
    {
        m_messageFunc = messageFunc;
        m_syncFunc = syncFunc;
    }

private:
    static void onPost(const OC::HeaderOptions &/*headerOption*/,
                const OC::OCRepresentation & /*rep*/ , const int /*eCode*/)
    {
    }
    void findResultCallback(std::shared_ptr<OC::OCResource> resource)
    {
        if(resource->uri() == "/notification")
        {
            resource->get(std::string("oic.wk.notification"), std::string("oic.if.baseline"),
                    OC::QueryParamsMap(), std::bind(&NSConsumerSimulator::onGet, this,
                            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                            resource), OC::QualityOfService::LowQos);
        }
    }
    void onGet(const OC::HeaderOptions &/*headerOption*/,
            const OC::OCRepresentation & /*rep*/ , const int /*eCode*/,
            std::shared_ptr<OC::OCResource> resource)
    {
        OC::QueryParamsMap map;
        map.insert(std::pair<std::string,std::string>(std::string("consumerid"),
                std::string("123456789012345678901234567890123456")));

        try
        {
            std::vector<std::string> rts{"oic.wk.notification"};

            m_msgResource
                = OC::OCPlatform::constructResourceObject(
                        std::string(resource->host()), std::string(resource->uri() + "/message"),
                        OCConnectivityType(resource->connectivityType()), true, rts,
                        std::vector<std::string>(resource->getResourceInterfaces()));

            m_msgResource->observe(OC::ObserveType::Observe, map,
                            std::bind(&NSConsumerSimulator::onObserve, this,
                                    std::placeholders::_1, std::placeholders::_2,
                                    std::placeholders::_3, std::placeholders::_4, resource),
                            OC::QualityOfService::LowQos);
        }
        catch(std::exception & e)
        {
            std::cout << "OC::ResoureInitException : " << e.what() << std::endl;
        }
        m_syncResource
            = OC::OCPlatform::constructResourceObject(resource->host(), resource->uri() + "/sync",
                    resource->connectivityType(), true, resource->getResourceTypes(),
                    resource->getResourceInterfaces());

        m_syncResource->observe(OC::ObserveType::Observe, map,
                std::bind(&NSConsumerSimulator::onObserve, this,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4, resource),
                OC::QualityOfService::LowQos);


        m_topicResource
            = OC::OCPlatform::constructResourceObject(resource->host(), resource->uri() + "/topic",
                    resource->connectivityType(), true, resource->getResourceTypes(),
                    resource->getResourceInterfaces());

    }
    void onObserve(const OC::HeaderOptions &/*headerOption*/,
            const OC::OCRepresentation &rep , const int & /*eCode*/, const int &,
            std::shared_ptr<OC::OCResource> )
    {
        if (rep.getUri() == "/notification/message" && rep.hasAttribute("messageid")
                && rep.getValue<int>("messageid") != 1)
        {
            m_messageFunc(int(rep.getValue<int>("messageid")),
                          std::string(rep.getValueToString("title")),
                          std::string(rep.getValueToString("contenttext")),
                          std::string(rep.getValueToString("source")));

            if(rep.getValue<int>("messageid") == 3)
            {
                m_topicResource->get(std::string("oic.wk.notification"),
                        std::string("oic.if.baseline"), OC::QueryParamsMap(),
                        std::bind(&NSConsumerSimulator::onTopicGet, this, std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3, m_topicResource),
                                OC::QualityOfService::LowQos);
            }
        }
        else if (rep.getUri() == "/notification/sync")
        {
            m_syncFunc(int(rep.getValue<int>("state")), int(rep.getValue<int>("messageid")));
        }
    }

    void onTopicGet(const OC::HeaderOptions &/*headerOption*/,
            const OC::OCRepresentation & rep , const int /*eCode*/,
            std::shared_ptr<OC::OCResource> /*resource*/)
    {

        if(!isTopicPost)
        {
            isTopicPost = true;
            OC::OCRepresentation postRep;

            std::vector<OC::OCRepresentation> topicArr =
                            rep.getValue<std::vector<OC::OCRepresentation>>("topiclist");

            std::vector<OC::OCRepresentation> postTopicArr;

            for(std::vector<OC::OCRepresentation>::iterator it = topicArr.begin();
                    it != topicArr.end(); ++it)
            {
                /* std::cout << *it; ... */
                OC::OCRepresentation topic = *it;
                OC::OCRepresentation postTopic;

                postTopic.setValue("topicname", topic.getValueToString("topicname"));
                postTopic.setValue("topicstate", (int) topic.getValue<int>("topicstate"));

                postTopicArr.push_back(topic);

//                std::cout << "tName : " << tName << std::endl;
//                std::cout << "tState : " << tState << std::endl;
            }

            postRep.setValue<std::vector<OC::OCRepresentation>>
                ("topiclist", postTopicArr);

            OC::QueryParamsMap map;
            map.insert(std::pair<std::string,std::string>(std::string("consumerid"),
                    std::string("123456789012345678901234567890123456")));
            m_topicResource->post(postRep, map, &onPost, OC::QualityOfService::LowQos);
        }
    }

    OCStackResult msgResourceCancelObserve(OC::QualityOfService qos)
    {
        return m_msgResource->cancelObserve(qos);
    }

    OCStackResult syncResourceCancelObserve(OC::QualityOfService qos)
    {
        return m_syncResource->cancelObserve(qos);
    }
};


#endif //_NS_CONSUMER_SIMULATOR_H_
