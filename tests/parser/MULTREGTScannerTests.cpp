/*
  Copyright 2014 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_MODULE MULTREGTScannerTests

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/Grid/MULTREGTScanner.hpp>

#include <opm/input/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FaceDir.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/M.hpp>

#include <array>
#include <initializer_list>
#include <stdexcept>
#include <vector>

BOOST_AUTO_TEST_CASE(TestRegionName) {
    BOOST_CHECK_EQUAL( "FLUXNUM" , Opm::MULTREGT::RegionNameFromDeckValue( "F"));
    BOOST_CHECK_EQUAL( "MULTNUM" , Opm::MULTREGT::RegionNameFromDeckValue( "M"));
    BOOST_CHECK_EQUAL( "OPERNUM" , Opm::MULTREGT::RegionNameFromDeckValue( "O"));

    BOOST_CHECK_THROW( Opm::MULTREGT::RegionNameFromDeckValue("o") , std::invalid_argument);
    BOOST_CHECK_THROW( Opm::MULTREGT::RegionNameFromDeckValue("X") , std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(TestNNCBehaviourEnum) {
    BOOST_CHECK_MESSAGE(Opm::MULTREGT::NNCBehaviourEnum::ALL == Opm::MULTREGT::NNCBehaviourFromString("ALL"),
                        R"(Behaviour("ALL") must be ALL)");

    BOOST_CHECK_MESSAGE(Opm::MULTREGT::NNCBehaviourEnum::NNC == Opm::MULTREGT::NNCBehaviourFromString("NNC"),
                        R"(Behaviour("NNC") must be NNC)");

    BOOST_CHECK_MESSAGE(Opm::MULTREGT::NNCBehaviourEnum::NONNC == Opm::MULTREGT::NNCBehaviourFromString("NONNC"),
                        R"(Behaviour("NONNC") must be NONNC)");

    BOOST_CHECK_MESSAGE(Opm::MULTREGT::NNCBehaviourEnum::NOAQUNNC == Opm::MULTREGT::NNCBehaviourFromString("NOAQUNNC"),
                        R"(Behaviour("NOAQUNNC") must be NOAQUNNC)");

    BOOST_CHECK_THROW(Opm::MULTREGT::NNCBehaviourFromString("Invalid"), std::invalid_argument);
}

namespace {
    Opm::Deck createInvalidMULTREGTDeck()
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 3 3 2 /
GRID
DX
18*0.25 /
DY
18*0.25 /
DZ
18*0.25 /
TOPS
9*0.25 /
FLUXNUM
1 1 2
1 1 2
1 1 2
3 4 5
3 4 5
3 4 5
/
MULTREGT
1  2   0.50   G   ALL    M / -- Invalid direction
/
MULTREGT
1  2   0.50   X   ALL    G / -- Invalid region
/
MULTREGT
1  2   0.50   X   ALL    M / -- Region not in deck
/
EDIT
)");
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(InvalidInput) {
    Opm::Deck deck = createInvalidMULTREGTDeck();
    Opm::EclipseGrid grid( deck );
    Opm::TableManager tm(deck);
    Opm::EclipseGrid eg( deck );
    Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, eg, tm);

    // Invalid direction
    std::vector<const Opm::DeckKeyword*> keywords0;
    const auto& multregtKeyword0 = deck["MULTREGT"][0];
    keywords0.push_back( &multregtKeyword0 );
    BOOST_CHECK_THROW( Opm::MULTREGTScanner scanner( grid, &fp, keywords0 ); , std::invalid_argument );

    // Not supported region
    std::vector<const Opm::DeckKeyword*> keywords1;
    const auto& multregtKeyword1 = deck["MULTREGT"][1];
    keywords1.push_back( &multregtKeyword1 );
    BOOST_CHECK_THROW( Opm::MULTREGTScanner scanner( grid, &fp, keywords1 ); , std::invalid_argument );

    // The keyword is ok; but it refers to a region which is not in the deck.
    std::vector<const Opm::DeckKeyword*> keywords2;
    const auto& multregtKeyword2 = deck["MULTREGT"][2];
    keywords2.push_back( &multregtKeyword2 );
    BOOST_CHECK_THROW( Opm::MULTREGTScanner scanner( grid, &fp, keywords2 ); , std::logic_error );
}

namespace {
    Opm::Deck createNotSupportedMULTREGTDeck()
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 3 3 2 /
GRID
DX
18*0.25 /
DY
18*0.25 /
DZ
18*0.25 /
TOPS
9*0.25 /
FLUXNUM
1 1 2
1 1 2
1 1 2
3 4 5
3 4 5
3 4 5
/
MULTREGT
1  2   0.50   X   NOAQUNNC  F / -- Not support NOAQUNNC behaviour
/
MULTREGT
2  2   0.50   X   ALL    M / -- Region values equal
/
EDIT
)");
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(NotSupported) {
    Opm::Deck deck = createNotSupportedMULTREGTDeck();
    Opm::EclipseGrid grid( deck );
    Opm::TableManager tm(deck);
    Opm::EclipseGrid eg( deck );
    Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, eg, tm);


    // Not support NOAQUNNC behaviour
    std::vector<const Opm::DeckKeyword*> keywords0;
    const auto& multregtKeyword0 = deck["MULTREGT"][0];
    keywords0.push_back( &multregtKeyword0 );
    BOOST_CHECK_THROW( Opm::MULTREGTScanner scanner( grid, &fp, keywords0 ); , std::invalid_argument );

    // srcValue == targetValue - not supported
    std::vector<const Opm::DeckKeyword*> keywords1;
    const Opm::DeckKeyword& multregtKeyword1 = deck["MULTREGT"][1];
    keywords1.push_back( &multregtKeyword1 );
    BOOST_CHECK_THROW( Opm::MULTREGTScanner scanner( grid, &fp, keywords1 ); , std::invalid_argument );
}

namespace {
    Opm::Deck createDefaultedRegions()
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
 3 3 2 /
GRID
DX
18*0.25 /
DY
18*0.25 /
DZ
18*0.25 /
TOPS
9*0.25 /
FLUXNUM
1 1 2
1 1 2
1 1 2
3 4 5
3 4 5
3 4 5
/
MULTREGT
3  4   1.25   XYZ   ALL    F /
2  -1   0   XYZ   ALL    F / -- Defaulted from region value
1  -1   0   XYZ   ALL    F / -- Defaulted from region value
2  1   1      XYZ   ALL    F / Override default
/
MULTREGT
2  *   0.75   XYZ   ALL    F / -- Defaulted to region value
/
EDIT
)");
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(DefaultedRegions) {
  Opm::Deck deck = createDefaultedRegions();
  Opm::EclipseGrid grid( deck );
  Opm::TableManager tm(deck);
  Opm::EclipseGrid eg( deck );
  Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, eg, tm);


  std::vector<const Opm::DeckKeyword*> keywords0;
  const auto& multregtKeyword0 = deck["MULTREGT"][0];
  keywords0.push_back( &multregtKeyword0 );
  Opm::MULTREGTScanner scanner0(grid, &fp, keywords0);
  BOOST_CHECK_EQUAL( scanner0.getRegionMultiplier(grid.getGlobalIndex(0,0,1), grid.getGlobalIndex(1,0,1), Opm::FaceDir::XPlus ), 1.25);
  BOOST_CHECK_EQUAL( scanner0.getRegionMultiplier(grid.getGlobalIndex(1,0,0), grid.getGlobalIndex(2,0,0), Opm::FaceDir::XPlus ), 1.0);
  BOOST_CHECK_EQUAL( scanner0.getRegionMultiplier(grid.getGlobalIndex(2,0,1), grid.getGlobalIndex(2,0,0), Opm::FaceDir::ZMinus ), 0.0);

  std::vector<const Opm::DeckKeyword*> keywords1;
  const Opm::DeckKeyword& multregtKeyword1 = deck["MULTREGT"][1];
  keywords1.push_back( &multregtKeyword1 );
  Opm::MULTREGTScanner scanner1(grid, &fp, keywords1 );
  BOOST_CHECK_EQUAL( scanner1.getRegionMultiplier(grid.getGlobalIndex(2,0,0), grid.getGlobalIndex(1,0,0), Opm::FaceDir::XMinus ), 0.75);
  BOOST_CHECK_EQUAL( scanner1.getRegionMultiplier(grid.getGlobalIndex(2,0,0), grid.getGlobalIndex(2,0,1), Opm::FaceDir::ZPlus), 0.75);
}

namespace {
    Opm::Deck createCopyMULTNUMDeck()
    {
        return Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
2 2 2 /
GRID
DX
8*0.25 /
DY
8*0.25 /
DZ
8*0.25 /
TOPS
4*0.25 /
FLUXNUM
1 2
1 2
3 4
3 4
/
COPY
 FLUXNUM  MULTNUM /
/
MULTREGT
1  2   0.50/
/
EDIT
)");
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(MULTREGT_COPY_MULTNUM) {
    Opm::Deck deck = createCopyMULTNUMDeck();
    Opm::TableManager tm(deck);
    Opm::EclipseGrid eg(deck);
    Opm::FieldPropsManager fp(deck, Opm::Phases{true, true, true}, eg, tm);

    BOOST_CHECK_NO_THROW(fp.has_int("FLUXNUM"));
    BOOST_CHECK_NO_THROW(fp.has_int("MULTNUM"));
    const auto& fdata = fp.get_global_int("FLUXNUM");
    const auto& mdata = fp.get_global_int("MULTNUM");
    std::vector<int> data = { 1, 2, 1, 2, 3, 4, 3, 4 };

    for (auto i = 0; i < 2 * 2 * 2; i++) {
        BOOST_CHECK_EQUAL(fdata[i], mdata[i]);
        BOOST_CHECK_EQUAL(fdata[i], data[i]);
    }
}
