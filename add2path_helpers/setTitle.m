function hTitle = setTitle(titleText, PosX, PosY)

hTitle  = text(PosX, PosY, ...
    titleText, ...
    'HorizontalAlignment','center');

set([hTitle], ...
    'FontName'   , 'Helvetica');

set( hTitle                    , ...
    'FontSize'   , 28          , ...
    'FontWeight' , 'bold'      );

end
