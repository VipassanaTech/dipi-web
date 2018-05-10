(function ($) {

	window.do_pincode = function ( code , country, state, city )
	{
		$.getJSON("/get-location-from-pincode",{ code: code }, function(data){
			if (data)
			{
				$(country).val(data[0].country);
				$(state).val(data[0].state);
				$(city).val(data[0].city);				
			}
		});
	}


	window.autocomplete_country = function ( country )
	{
		$(document).on("keydown.autocomplete", country, function(){
			$(this).autocomplete({
				source: function( request, response ) {
					$(country).addClass( "throbbing" );
			        $.ajax({
			          url: "/autocomplete/get-country/" + request.term,
			          dataType: "json",
			          success: function( data ) {
			          	$(country).removeClass( "throbbing" );
			            response( data );
			          }
			        });
				},
	  			minLength: 1,
	  			select: function( event, ui ) {
	        		//console.log( "Selected: " + ui.item.value + " aka " + ui.item.code );
	        		$( country ).val( ui.item.code );
	        		return false;
	      		}
      		});
		});
	}

	window.autocomplete_state = function ( state, country )
	{
		$(document).on("keydown.autocomplete", state, function(){
			$(state).autocomplete({
				source: function( request, response ) {
					$(state).addClass( "throbbing" );
			        $.ajax({
			          url: "/autocomplete/get-state/" + request.term,
			          dataType: "json",
			          data: {country: $(country).val()},
			          success: function( data ) {
			          	$(state).removeClass( "throbbing" );
			            response( data );
			          }
			        });
				},
	  			minLength: 1,
	  			select: function( event, ui ) {
	        		$( state ).val( ui.item.code );
	        		return false;
	      		}
			});
     	});

	}

	window.autocomplete_city = function ( city, country, state )
	{
		$(document).on("keydown.autocomplete", city, function(){
			$(city).autocomplete({
				source: function( request, response ) {
					$(city).addClass( "throbbing" );
			        $.ajax({
			          url: "/autocomplete/get-city/" + request.term,
			          dataType: "json",
			          data: {country: $(country).val(), state: $(state).val()},
			          success: function( data ) {
			          	$(city).removeClass( "throbbing" );
			            response( data );
			          }
			        });
				},
	  			minLength: 1,
			});
		});

	}

})(jQuery);
