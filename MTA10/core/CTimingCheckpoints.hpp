/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.0
*  LICENSE:     See LICENSE in the top level directory
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

//
// Temp file for debugging purposes only
//

//
// Class to record the CPU time used at various points in the game
//
class CTimingCheckpoints
{
public:
    struct SCheckpointItem
    {
        TIMEUS timeUs;
        SString strTag;
    };

    bool    m_bEnabled;
    TIMEUS  m_CheckpointsStartUs;
    TIMEUS  m_FrameStartTimeUs;
    TIMEUS  m_PrevFrameTimeUs;
    SString m_LogFileName;
    SString m_strDetail;
    SString m_strDetailForce;
    std::vector < SCheckpointItem > m_CheckpointList;
    std::vector < SString >         m_InsideList;

    CTimingCheckpoints ( void )
    {
        m_bEnabled = false;
        m_FrameStartTimeUs = 0;
        m_PrevFrameTimeUs = 0;
        m_CheckpointsStartUs = 0;
        m_LogFileName = CalcMTASAPath ( "timings.log" ) ;
    }


    ////////////////////////////////////////
    //
    // Called at start of frame
    //
    ////////////////////////////////////////
    void BeginTimingCheckpoints ( void )
    {
        bool bEnabled = ( g_pCore->GetDiagnosticDebug () == EDiagnosticDebug::LOG_TIMING_0000 );

        if ( bEnabled != m_bEnabled )
        {
            m_bEnabled = bEnabled;
            if ( bEnabled )
            {
                ClearLog ();
                AppendLog ( SString ( "Started timing checkpoints [Ver:%d.%d.%d-%d.%05d] [Date:%s]"
                                    ,MTASA_VERSION_MAJOR, MTASA_VERSION_MINOR, MTASA_VERSION_MAINTENANCE, MTASA_VERSION_TYPE, MTASA_VERSION_BUILD
                                    ,*GetLocalTimeString ( true ).SplitLeft ( " " ) ) );
            }
            else
                AppendLog ( "Stopped timing checkpoints" );
        }

        if ( !m_bEnabled )
            return;

        m_FrameStartTimeUs = GetTimeUs ();
        m_CheckpointsStartUs = GetTimeUs ();
        m_CheckpointList.clear ();
        m_InsideList.clear ();
        m_strDetailForce = "";
        m_strDetail = "";
    }


    ////////////////////////////////////////
    //
    // Called at end of frame
    //
    ////////////////////////////////////////
    void EndTimingCheckpoints ( void )
    {
        TIMEUS frameTimeUs = GetTimeUs() - m_FrameStartTimeUs;

        // Record if frame over 30ms and over double previous frame
        if ( frameTimeUs > (1000/30*1000) && frameTimeUs > m_PrevFrameTimeUs * 2 )
        {
            if ( m_bEnabled )
            {
                AppendLog ( SString ( "--------------CCore::ApplyFrameRateLimit frame time: %1.3fms    (prev %1.3fms)", (float)frameTimeUs/1000.f, (float)m_PrevFrameTimeUs/1000.f ) );
                DumpTimingCheckpoints();
            }
        }
        m_PrevFrameTimeUs = frameTimeUs;
    }


    ////////////////////////////////////////
    //
    // Called during frame at point of interest
    //
    ////////////////////////////////////////
    void OnTimingCheckpoint ( const char* szTag )
    {
        if ( !m_bEnabled )
            return;

        SCheckpointItem item;
        item.timeUs = GetTimeUs ();
        item.strTag = szTag;
        m_CheckpointList.push_back ( item );
    }


    ////////////////////////////////////////
    //
    // Called during frame at point of interest
    //
    ////////////////////////////////////////
    void OnTimingDetail ( const char* szTag, bool bForce )
    {
        if ( !m_bEnabled )
            return;
        if ( bForce )
            m_strDetailForce += szTag + SStringX ( "\n" );
        else
            m_strDetail += szTag + SStringX ( "\n" );
    }

    //
    // Timing sections
    //
    struct SSectionInfo
    {
        SSectionInfo ( void ) : iEnterCount ( 0 ), iLeaveCount ( 0 ), totalTime ( 0 ) {}
        int iEnterCount;
        int iLeaveCount;
        TIMEUS totalTime;
        SString strName;
        std::vector < TIMEUS > enterTimes;
    };
    std::map < SString, SSectionInfo > sectionInfoMap;

    SSectionInfo& GetSectionInfo ( const SString& strName )
    {
        SSectionInfo* pInfo = MapFind ( sectionInfoMap, strName );
        if ( !pInfo )
        {
            pInfo = &MapGet ( sectionInfoMap, strName );
            pInfo->strName = strName;
        }
        return *pInfo;
    }

    void EnterSection ( const SString& strName, TIMEUS timeStamp )
    {
        SSectionInfo& info = GetSectionInfo ( strName );
        info.iEnterCount++;
        info.enterTimes.push_back ( timeStamp );
    }

    void LeaveSection ( const SString& strName, TIMEUS timeStamp )
    {
        SSectionInfo& info = GetSectionInfo ( strName );
        info.iLeaveCount++;
        if ( !info.enterTimes.empty () )
        {
            TIMEUS prevTimeStamp = info.enterTimes.back ();
            info.enterTimes.pop_back ();
            info.totalTime += timeStamp - prevTimeStamp;
        }
    }


    ////////////////////////////////////////
    //
    // Output last frame stats
    //
    ////////////////////////////////////////
    void DumpTimingCheckpoints ( void )
    {
        if ( !m_bEnabled )
            return;

        // Compile stats
        sectionInfoMap.clear ();
        TIMEUS base = m_CheckpointsStartUs;
        for ( uint i = 0 ; i < m_CheckpointList.size () ; i++ )
        {
            const SCheckpointItem& item = m_CheckpointList[i];
            TIMEUS delta = item.timeUs - m_CheckpointsStartUs;
            base = item.timeUs;

            const SString& strTag = item.strTag;
            if ( !strTag.empty () )
            {
                if ( strTag[0] == '+' )
                {
                    EnterSection( item.strTag.SubStr ( 1 ), delta );
                }
                else
                if ( strTag[ strTag.length () - 1 ] == '+' )
                {
                    EnterSection( strTag.Left ( strTag.length () - 1 ), delta );
                }
                else
                if ( strTag[0] == '-' )
                {
                    LeaveSection( item.strTag.SubStr ( 1 ), delta );
                }
                else
                if ( strTag[ strTag.length () - 1 ] == '-' )
                {
                    LeaveSection( strTag.Left ( strTag.length () - 1 ), delta );
                }
                else
                {
                    EnterSection( item.strTag, delta );
                    LeaveSection( item.strTag, delta + 1 );
                }
            }
        }

        // Sort by time
        std::vector < SSectionInfo* > resultList;
        for ( std::map < SString, SSectionInfo >::iterator iter = sectionInfoMap.begin () ; iter != sectionInfoMap.end () ; ++iter )
        {
            resultList.push_back ( &iter->second );
        }

        sort_inline ( resultList.begin (), resultList.end (), ( const SSectionInfo* a, const SSectionInfo* b ) { return a->totalTime > b->totalTime; } );


        // Output to string
        SString strStatus;
        for ( uint i = 0 ; i < resultList.size () ; i++ )
        {
            SSectionInfo& info = *resultList[i];
            if ( info.totalTime < 5000 )
                break;
            if ( info.iEnterCount == info.iLeaveCount )
                strStatus += SString ( "%1.1fms [%d] %s   |  ", info.totalTime/1000.f, info.iEnterCount, *info.strName ) ;
            else
                strStatus += SString ( "%1.1fms [%d/%d] %s   |  ", info.totalTime/1000.f, info.iEnterCount, info.iLeaveCount, *info.strName ) ;
        }

        if ( !m_strDetailForce.empty () )
            AppendLog ( m_strDetailForce.TrimEnd ( "\n" ) );

        if ( !strStatus.empty () )
        {
            if ( !m_strDetail.empty () )
                AppendLog ( m_strDetail.TrimEnd ( "\n" ) );

            AppendLog ( strStatus );
        }
    }


    ////////////////////////////////////////
    //
    // Output to log file
    //
    ////////////////////////////////////////
    void AppendLog ( const SString& strStatus )
    {
        FileAppend ( m_LogFileName, GetLocalTimeString ( false, true ) + " - " + strStatus + "\n", false );
    }


    ////////////////////////////////////////
    //
    // Delete log file
    //
    ////////////////////////////////////////
    void ClearLog ( void )
    {
        FileDelete ( m_LogFileName );
    }

};

CTimingCheckpoints ms_TimingCheckpoints;